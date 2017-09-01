
#include"main.hpp"
#include"Option.hpp"
#include"Pre.hpp"
#include"Module.hpp"
#include"Mcmc.hpp"
#include"Est.hpp"
#include"nlopt.hpp"



double logcond_fn(const int p,const int t,const vector<double>& Phi_t,const Mcmc& d_mc,const Module& mo,const bool b=false)
{
    double f0=0;
    if (mo.modulebool() and mo.adj().at(p).size()>0) {
        vector<double> CPSumtop;
        for (set<int>::const_iterator it=mo.adj().at(p).begin();it!=mo.adj().at(p).end();it++) {
            CPSumtop.push_back(d_mc.CPSum().at(*it).at(t));
        }
        f0 = accumulate(CPSumtop.begin(),CPSumtop.end(),0.)/CPSumtop.size();
        f0 *= Phi_t.at(1);
    }
    const double F = Phi_t.at(0)+f0;
    if (b) return F;
    return d_mc.CPSum().at(p).at(t)*F - d_mc.op().ns()*log(1 + exp(F));
}


class Logpseudol
{
    const Mcmc d_mc;
    const Module d_mo;
    public:
    Logpseudol(const Mcmc& mc,const Module& mo) : d_mc(mc),d_mo(mo) { }
    double operator()(const vector<double>& d_Phi, vector<double> &grad) const
    {
        double logpseudo=0;
        for (int t=1;t<d_mc.op().nt()-1;t++) for (unsigned p=0;p<d_mc.xx().size();p++) {
            logpseudo += logcond_fn(p,t,d_Phi,d_mc,d_mo);
        }
        return logpseudo;
    }
    static double wrap(const vector<double> &x, vector<double> &grad, void *data)
    {
        return (*reinterpret_cast<Logpseudol*>(data))(x, grad); 
    }
};


Est::Est(const Mcmc& mc,const Module& mo) : d_mc(mc),d_mo(mo)
{
    ofstream ofs("param.txt");
    if (not ofs) throw runtime_error("can't open param.txt");
    d_Phi.assign(2,1e-3);
    nlopt::opt opt(nlopt::LN_COBYLA,Phi().size());
    vector<double> lb(Phi().size(),1e-9); lb.at(0) = -10;
    opt.set_lower_bounds(lb);
    opt.set_upper_bounds(vector<double>(Phi().size(),10));
    opt.set_maxeval(10000);
    opt.set_xtol_abs(1e-6);

    Logpseudol obj_fn(d_mc,d_mo);
    opt.set_max_objective(Logpseudol::wrap, &obj_fn);
    double maxf;
    nlopt::result result = opt.optimize(d_Phi, maxf);
    if (result<0) throw runtime_error("result");
    ofs<<"gamma = "<<Phi().at(0)<<" beta = "<<Phi().at(1)<<'\n';
    cout<<"gamma = "<<Phi().at(0)<<" beta = "<<Phi().at(1)<<'\n';
    d_varphi.assign(d_mc.xx().size(),vector<double>(d_mc.op().nt()));
    for (int t=1;t<d_mc.op().nt()-1;t++) {
        for (unsigned p=0;p<d_mc.xx().size();p++) {
            d_varphi.at(p).at(t)= logcond_fn(p,t,Phi(),d_mc,d_mo,true);
        }
    }
}
