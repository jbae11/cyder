/*! \file DegRateNuclide.cpp
    \brief Implements the DegRateNuclide class used by the Generic Repository 
    \author Kathryn D. Huff
 */
#include <iostream>
#include <fstream>
#include <deque>
#include <time.h>
#include <assert.h>
#include <boost/lexical_cast.hpp>

#include "CycException.h"
#include "Logger.h"
#include "Timer.h"
#include "DegRateNuclide.h"
#include "Material.h"

using namespace std;
using boost::lexical_cast;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DegRateNuclide::DegRateNuclide():
  deg_rate_(0),
  bc_type_(LAST_BC_TYPE),
  v_(0),
  tot_deg_(0),
  last_degraded_(-1)
{
  wastes_ = deque<mat_rsrc_ptr>();

  set_geom(GeometryPtr(new Geometry()));
  last_updated_=0;

  vec_hist_ = VecHist();
  conc_hist_ = ConcHist();
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DegRateNuclide::DegRateNuclide(QueryEngine* qe):
  deg_rate_(0),
  bc_type_(LAST_BC_TYPE),
  v_(0),
  tot_deg_(0),
  last_degraded_(-1)
{
  wastes_ = deque<mat_rsrc_ptr>();
  vec_hist_ = VecHist();
  conc_hist_ = ConcHist();

  set_geom(GeometryPtr(new Geometry()));
  last_updated_=0;

  initModuleMembers(qe);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DegRateNuclide::~DegRateNuclide(){
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DegRateNuclide::initModuleMembers(QueryEngine* qe){
  set_v(lexical_cast<double>(qe->getElementContent("advective_velocity")));
  set_deg_rate(lexical_cast<double>(qe->getElementContent("degradation")));
  QueryEngine* bc_type_qe = qe->queryElement("bc_type");
  string bc_type_string;
  list <string> choices;
  list <string>::iterator it;
  choices.push_back("CAUCHY");
  choices.push_back("DIRICHLET");
  choices.push_back("SOURCE_TERM");
  choices.push_back("NEUMANN");
  for( it=choices.begin(); it!=choices.end(); ++it) {
    if( bc_type_qe->nElementsMatchingQuery(*it) == 1){
      set_bc_type(enumerateBCType(*it));
    }
  }
  LOG(LEV_DEBUG2,"GRDRNuc") << "The DegRateNuclide Class initModuleMembers(qe) function has been called";;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
NuclideModelPtr DegRateNuclide::copy(const NuclideModel& src){
  const DegRateNuclide* src_ptr = dynamic_cast<const DegRateNuclide*>(&src);

  set_deg_rate(src_ptr->deg_rate());
  set_bc_type(src_ptr->bc_type());
  set_v(src_ptr->v());
  set_tot_deg(0);
  set_last_degraded(-1);

  // copy the geometry AND the centroid. It should be reset later.
  set_geom(GeometryPtr(new Geometry()));
  geom_->copy(src_ptr->geom(), src_ptr->geom()->centroid());

  wastes_ = deque<mat_rsrc_ptr>();
  vec_hist_ = VecHist();
  conc_hist_ = ConcHist();

  return shared_from_this();
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
void DegRateNuclide::updateNuclideParamsTable(){
  shared_from_this()->addRowToNuclideParamsTable("degradation", deg_rate());
  shared_from_this()->addRowToNuclideParamsTable("advective_velocity", v());
  shared_from_this()->addRowToNuclideParamsTable("ref_disp", mat_table_->ref_disp());
  shared_from_this()->addRowToNuclideParamsTable("ref_kd", mat_table_->ref_kd());
  shared_from_this()->addRowToNuclideParamsTable("ref_sol", mat_table_->ref_sol());
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
void DegRateNuclide::update(int the_time){
  update_vec_hist(the_time);
  update_conc_hist(the_time);
  set_last_updated(the_time);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
void DegRateNuclide::print(){
    LOG(LEV_DEBUG2,"GRDRNuc") << "DegRateNuclide Model";;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
void DegRateNuclide::absorb(mat_rsrc_ptr matToAdd)
{
  // Get the given DegRateNuclide's contaminant material.
  // add the material to it with the material absorb function.
  // each nuclide model should override this function
  LOG(LEV_DEBUG2,"GRDRNuc") << "DegRateNuclide is absorbing material: ";
  matToAdd->print();
  wastes_.push_back(matToAdd);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
mat_rsrc_ptr DegRateNuclide::extract(const CompMapPtr comp_to_rem, double kg_to_rem)
{
  // Get the given DegRateNuclide's contaminant material.
  // add the material to it with the material extract function.
  // each nuclide model should override this function
  LOG(LEV_DEBUG2,"GRDRNuc") << "DegRateNuclide" << "is extracting composition: ";
  comp_to_rem->print() ;
  mat_rsrc_ptr to_ret = mat_rsrc_ptr(MatTools::extract(comp_to_rem, kg_to_rem, 
        wastes_, 1e-16));
  update(last_updated());
  return to_ret;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
void DegRateNuclide::transportNuclides(int the_time){
  // This should transport the nuclides through the component.
  // It will likely rely on the internal flux and will produce an external flux. 
  update_degradation(the_time, deg_rate());
  update(the_time);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
void DegRateNuclide::set_deg_rate(double cur_rate){
  if( cur_rate < 0 || cur_rate > 1 ) {
    stringstream msg_ss;
    msg_ss << "The DegRateNuclide degradation rate range is 0 to 1, inclusive.";
    msg_ss << " The value provided was ";
    msg_ss << cur_rate;
    msg_ss <<  ".";
    LOG(LEV_ERROR,"GRDRNuc") << msg_ss.str();;
    throw CycRangeException(msg_ss.str());
  } else {
    deg_rate_ = cur_rate;
  }
  assert((cur_rate >=0) && (cur_rate <= 1));
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
double DegRateNuclide::contained_mass(){
  return shared_from_this()->contained_mass(last_degraded());
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
pair<IsoVector, double> DegRateNuclide::source_term_bc(){
  pair<IsoVector, double> curr_mats;
  curr_mats=MatTools::sum_mats(wastes_);
  return make_pair(curr_mats.first, tot_deg()*curr_mats.second);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
IsoConcMap DegRateNuclide::dirichlet_bc(){
  IsoConcMap dirichlet;
  pair<IsoVector, double> st = shared_from_this()->source_term_bc();
  dirichlet = MatTools::comp_to_conc_map(CompMapPtr(st.first.comp()), st.second, V_ff());
  return dirichlet;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
ConcGradMap DegRateNuclide::neumann_bc(IsoConcMap c_ext, Radius r_ext){
  ConcGradMap to_ret;

  IsoConcMap c_int = dirichlet_bc();
  Radius r_int = geom_->radial_midpoint();

  int iso; 
  IsoConcMap::iterator it;
  for( it=c_int.begin(); it != c_int.end(); ++it){
    iso = (*it).first;
    Elem elem = int(iso/1000);
    if( c_ext.count(iso) != 0) {  
      // in both
      to_ret[iso] = calc_conc_grad(c_ext[iso], c_int[iso], r_ext, r_int);
    } else {  
      // in c_int_only
      to_ret[iso] = calc_conc_grad(0, c_int[iso], r_ext, r_int);
    }
  }
  for( it=c_ext.begin(); it != c_ext.end(); ++it){
    iso = (*it).first;
    Elem elem = int(iso/1000);
    if( c_int.count(iso) == 0) { 
      // in c_ext only
      to_ret[iso] = calc_conc_grad(c_ext[iso], 0, r_ext, r_int);
    }
  }

  return to_ret;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
IsoFluxMap DegRateNuclide::cauchy_bc(IsoConcMap c_ext, Radius r_ext){
  // -D dC/dx + v_xC = v_x C
  IsoFluxMap to_ret;
  ConcGradMap neumann = neumann_bc(c_ext, r_ext);
  ConcGradMap::iterator it;
  Iso iso;
  Elem elem;
  for( it = neumann.begin(); it != neumann.end(); ++it){
    iso = (*it).first;
    elem = int(iso/1000);
    to_ret.insert(make_pair(iso, -mat_table_->D(elem)*(*it).second + v()*shared_from_this()->dirichlet_bc(iso)));
  }
  return to_ret;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
IsoConcMap DegRateNuclide::update_conc_hist(int the_time){
  return update_conc_hist(the_time, wastes_);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
IsoConcMap DegRateNuclide::update_conc_hist(int the_time, deque<mat_rsrc_ptr> mats){
  assert(last_degraded() <= the_time);
  assert(last_updated() <= the_time);

  IsoConcMap to_ret;

  pair<IsoVector, double> sum_pair; 
  sum_pair = MatTools::sum_mats(mats);

  if(sum_pair.second != 0 && geom_->volume() != numeric_limits<double>::infinity()) { 
    double scale = sum_pair.second/geom_->volume();
    CompMapPtr curr_comp = sum_pair.first.comp();
    CompMap::const_iterator it;
    it=(*curr_comp).begin();
    while(it != (*curr_comp).end() ) {
      int iso((*it).first);
      double conc((*it).second);
      to_ret.insert(make_pair(iso, conc*scale));
      ++it;
    }
  } else {
    to_ret[ 92235 ] = 0; 
  }
  conc_hist_[the_time] = to_ret;
  return to_ret;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
double DegRateNuclide::update_degradation(int the_time, double cur_rate){
  assert(last_degraded() <= the_time);
  if( last_degraded() == -1 ){ 
    set_last_degraded(the_time);
  } 
  if(cur_rate != deg_rate()){
    set_deg_rate(cur_rate);
  }
  double total = tot_deg() + deg_rate()*(the_time - last_degraded());
  set_tot_deg(min(1.0, total));
  assert(tot_deg_ <= 1.0);
  set_last_degraded(the_time);
  return tot_deg_;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
void DegRateNuclide::update_vec_hist(int the_time){
  vec_hist_[ the_time ] = MatTools::sum_mats(wastes_) ;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
void DegRateNuclide::update_inner_bc(int the_time, std::vector<NuclideModelPtr> daughters){
  std::vector<NuclideModelPtr>::iterator daughter;

  for( daughter = daughters.begin(); daughter!=daughters.end(); ++daughter){
    pair<CompMapPtr, double> comp_pair;
    std::pair<IsoVector, double> source_term;
    CompMapPtr comp_to_ext;
    double kg_to_ext=0;
    switch (bc_type_) {
      case SOURCE_TERM :
        source_term = (*daughter)->source_term_bc();
        if(source_term.second > 1e-30){
          comp_to_ext = CompMapPtr(source_term.first.comp());
          kg_to_ext=source_term.second;
        }
        break;
      case DIRICHLET :
        comp_pair = inner_dirichlet(*daughter);
        comp_to_ext = CompMapPtr(comp_pair.first);
        kg_to_ext = comp_pair.second;
      case NEUMANN :
        comp_pair = inner_neumann(*daughter);
        comp_to_ext = CompMapPtr(comp_pair.first);
        kg_to_ext = comp_pair.second;
        break;
      case CAUCHY :
        comp_pair = inner_cauchy(*daughter);
        comp_to_ext = CompMapPtr(comp_pair.first);
        kg_to_ext = comp_pair.second;
        break;
      default :
        // throw an error
        break;
    }
    if(kg_to_ext > 0) {
      assert(kg_to_ext <= (*daughter)->source_term_bc().second);
      shared_from_this()->absorb(mat_rsrc_ptr((*daughter)->extract(CompMapPtr(comp_to_ext), kg_to_ext)));
    }
  }
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
pair<CompMapPtr, double> DegRateNuclide::inner_neumann(NuclideModelPtr daughter){
  IsoConcMap conc_map;
  ConcGradMap grad_map;
  pair<CompMapPtr, double> comp_pair;
  //flux area perpendicular to flow, timeps porosit, times D.
  double int_factor =2*SECSPERMONTH*(daughter->geom()->length())*(daughter->geom()->outer_radius());;
  grad_map = daughter->neumann_bc(dirichlet_bc(), geom()->radial_midpoint());
  conc_map = MatTools::scaleConcMap(grad_map, tot_deg()*int_factor);
  IsoConcMap disp_map;
  IsoConcMap::iterator it;
  int iso;
  for(it=conc_map.begin(); it!=conc_map.end(); ++it) {
    if((*it).second < 0.0){
      iso=(*it).first;
      disp_map[iso] = -mat_table_->D(iso/1000.)*(*it).second;
    }
  }
  comp_pair = MatTools::conc_to_comp_map(disp_map, 1);
  return comp_pair; 
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
pair<CompMapPtr, double> DegRateNuclide::inner_dirichlet(NuclideModelPtr daughter){
  IsoConcMap conc_map;
  pair<CompMapPtr, double> comp_pair;
  //flux area perpendicular to flow, times v.
  
  double int_factor =2*SECSPERMONTH*v()*(daughter->geom()->length())*(daughter->geom()->outer_radius());;
  conc_map = MatTools::scaleConcMap(daughter->dirichlet_bc(), int_factor);
  IsoConcMap::iterator it;
  for(it=conc_map.begin(); it!=conc_map.end(); ++it) {
    if((*it).second < 0.0){
      (*it).second = 0.0;
    }
  }
  comp_pair = MatTools::conc_to_comp_map(conc_map, 1);
  return comp_pair; 
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
pair<CompMapPtr, double> DegRateNuclide::inner_cauchy(NuclideModelPtr daughter){
  pair<CompMapPtr, double> to_ret;
  pair<CompMapPtr, double> neumann = inner_neumann(daughter);
  pair<CompMapPtr, double> dirichlet = inner_dirichlet(daughter);
  double n_kg = neumann.second;
  double d_kg = dirichlet.second;
  IsoVector n_vec = IsoVector(neumann.first);
  IsoVector d_vec = IsoVector(dirichlet.first);
  n_vec.mix(d_vec, n_kg/d_kg);
  return make_pair(CompMapPtr(n_vec.comp()),n_kg+d_kg);
}


