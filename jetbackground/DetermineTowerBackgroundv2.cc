#include "DetermineTowerBackgroundv2.h"

#include "TowerBackground.h"
#include "TowerBackgroundv1.h"

#include <calobase/RawTower.h>
#include <calobase/RawTowerContainer.h>
#include <calobase/RawTowerDefs.h>
#include <calobase/RawTowerGeom.h>
#include <calobase/RawTowerGeomContainer.h>
#include <calobase/TowerInfo.h>
#include <calobase/TowerInfoContainer.h>

#include <ffaobjects/EventHeader.h>
#include <ffaobjects/EventHeaderv1.h>


#include <eventplaneinfo/Eventplaneinfo.h>
#include <eventplaneinfo/EventplaneinfoMap.h>

#include <globalvertex/GlobalVertexv3.h>
#include <globalvertex/GlobalVertexMapv1.h>

#include <centrality/CentralityInfo.h>

#include <ffamodules/CDBInterface.h>
#include <cdbobjects/CDBTTree.h>

#include <jetbase/Jet.h>
#include <jetbase/JetContainer.h>

#include <fun4all/Fun4AllReturnCodes.h>
#include <fun4all/SubsysReco.h>

#include <phool/PHCompositeNode.h>
#include <phool/PHIODataNode.h>
#include <phool/PHNode.h>
#include <phool/PHNodeIterator.h>
#include <phool/PHObject.h>
#include <phool/getClass.h>
#include <phool/phool.h>

#include <cmath>
#include <iostream>
#include <algorithm>
#include <map>
#include <utility>
#include <vector>
#include <set>
#include <string>
#include <cassert>

DetermineTowerBackgroundv2::DetermineTowerBackgroundv2(const std::string &name)
  : SubsysReco(name)
{}

int DetermineTowerBackgroundv2::InitRun(PHCompositeNode *topNode)
{
  if (m_flow_mode == 2)
  {
    if (Verbosity())
	  {
	    std::cout << "Loading the average calo v2" << std::endl;
	  }
    if ( LoadCalibrations() )
	  {
	    std::cout << "Load calibrations failed." << std::endl;
	    return Fun4AllReturnCodes::ABORTRUN;
	  }

  }
  
  std::cout << "USING LOCAL BUILD\n\n";
  return CreateNode(topNode);
}

int DetermineTowerBackgroundv2::LoadCalibrations()
{
  
  CDBTTree * cdbtree_calo_v2 = nullptr;

  std::string calibdir = CDBInterface::instance()->getUrl(m_calib_name);
  if (!m_overwrite_average_calo_v2_path.empty())
  {
    calibdir = m_overwrite_average_calo_v2_path;
  }

  if (calibdir.empty())
  {
    std::cout << "Could not find filename for calo average v2, exiting" << std::endl;
    exit(-1);
  }

  cdbtree_calo_v2 = new CDBTTree(calibdir);
  
    
  cdbtree_calo_v2->LoadCalibrations();

  m_cent_avg_v2.assign(100,0);

  for (int icent = 0; icent < 100; icent++)
  {
    m_cent_avg_v2[icent] = cdbtree_calo_v2->GetFloatValue(icent, "jet_calo_v2");
  }
      
  delete cdbtree_calo_v2;

  return Fun4AllReturnCodes::EVENT_OK;
}

int DetermineTowerBackgroundv2::get_zvrtx(PHCompositeNode *topNode)
{
  m_vtxz = 0;  // default to 0
  GlobalVertex *vtx = nullptr;
  auto * vertexmap = findNode::getClass<GlobalVertexMap>(topNode, "GlobalVertexMap");
  if ( !vertexmap )
  {
    std::cout << "DetermineTowerBackgroundv2::get_zvrtx - Fatal Error - GlobalVertexMap node is missing. Please turn on the do_global flag in the main macro in order to reconstruct the global vertex." << std::endl;
    exit(1);
  }

  if ( vertexmap->empty() )
  {
    if (Verbosity() > 0)
    {
      std::cout << "DetermineTowerBackgroundv2::get_zvrtx - empty vertex map, continuing as if zvtx = 0" << std::endl;
    }
    return Fun4AllReturnCodes::EVENT_OK;
  }

  auto vertices = vertexmap -> get_gvtxs_with_type( m_vertex_type );
  if( !vertices.empty() )
  {
    vtx = vertices.at(0);
  }
  else 
  {
    vtx = vertexmap->begin()->second;
  }

  if (vtx)
  {
    m_vtxz = vtx->get_z();
  }

  if ( std::isnan(m_vtxz) || std::abs(m_vtxz) > 1e3 )
  {
    static bool once = true;
    if ( once )
    {
      once = false;
      std::cout << "DetermineTowerBackgroundv2::get_zvrtx - WARNING - vertex is NAN. Continue with zvtx = 0 (further vertex warning will be suppressed)." << std::endl;
    }
    m_vtxz = 0;
  }

  if (Verbosity() > 1)
  {
    std::cout << "DetermineTowerBackgroundv2::get_zvrtx - zvtx = " << m_vtxz << std::endl;
  }
  return Fun4AllReturnCodes::EVENT_OK;

}

TowerInfoContainer * DetermineTowerBackgroundv2::LoadTowerInfoContainer(PHCompositeNode *topNode, const std::string &tower_node_name)
{
  auto * tic = findNode::getClass<TowerInfoContainer>(topNode, tower_node_name.c_str());
  if (!tic)
  {
    std::cout << "DetermineTowerBackgroundv2::LoadTowerInfoContainer - cannot find " << tower_node_name << ", exiting" << std::endl;
    exit(1);
  }
  return tic;
}

RawTowerGeomContainer * DetermineTowerBackgroundv2::LoadTowerGeomContainer(PHCompositeNode *topNode, const std::string &tower_geom_node_name)
{
  auto * tg = findNode::getClass<RawTowerGeomContainer>(topNode, tower_geom_node_name.c_str());
  if (!tg)
  {
    std::cout << "DetermineTowerBackgroundv2::LoadTowerGeomContainer - cannot find " << tower_geom_node_name << ", exiting" << std::endl;
    exit(1);
  }
  return tg;
}


int DetermineTowerBackgroundv2::fill_energy_vectors(PHCompositeNode *topNode, Jet::SRC src)
{

  // double tower_r = 0.0;
  m_towerinfos = nullptr;
  m_towergeom = nullptr;
  m_caloid = RawTowerDefs::CalorimeterId::NONE;
  if ( src == Jet::SRC::HCALIN_TOWERINFO )
  {
    m_towerinfos = LoadTowerInfoContainer(topNode, m_ihcal_towerinfo_node);
    m_towergeom  = LoadTowerGeomContainer(topNode, m_ihcal_geom_node);
    m_caloid     = RawTowerDefs::CalorimeterId::HCALIN;
    // tower_r = m_ihcal_r;
  }
  else if ( src == Jet::SRC::HCALOUT_TOWERINFO )
  {
    m_towerinfos  = LoadTowerInfoContainer(topNode, m_ohcal_towerinfo_node);
    m_towergeom   = LoadTowerGeomContainer(topNode, m_ohcal_geom_node);
    m_caloid      = RawTowerDefs::CalorimeterId::HCALOUT;
    // tower_r = m_ohcal_r;
  }
  else if ( src == Jet::SRC::CEMC_TOWERINFO_RETOWER  )
  {
    m_towerinfos = LoadTowerInfoContainer(topNode, m_cemc_retowerinfo_node);
    m_towergeom  = LoadTowerGeomContainer(topNode, m_ihcal_geom_node);
    m_caloid     = RawTowerDefs::CalorimeterId::HCALIN;
    // tower_r = m_emcal_r;
  }
  else if ( src == Jet::SRC::CEMC_TOWERINFO  )
  {
    m_towerinfos = LoadTowerInfoContainer(topNode, m_cemc_towerinfo_node);
    m_towergeom  = LoadTowerGeomContainer(topNode, m_cemc_geom_node);
    m_caloid     = RawTowerDefs::CalorimeterId::CEMC;
    // tower_r = m_emcal_r;
  }


  int ntowers = m_towerinfos -> size();
  for ( int ich = 0; ich < ntowers; ich++ )
  {
    auto * ti = m_towerinfos -> get_tower_at_channel(ich);
    assert(ti);

    const bool is_bad = !ti -> get_isGood();

    auto tkey             = m_towerinfos -> encode_key( ich );
    int comp_ieta         = m_towerinfos -> getTowerEtaBin( tkey );
    int comp_iphi         = m_towerinfos -> getTowerPhiBin( tkey );
    const auto key        = RawTowerDefs::encode_towerid( m_caloid, comp_ieta, comp_iphi );
    
    auto * tg             = m_towergeom -> get_tower_geometry( key );
    assert(tg);
   
    double comp_phi   = std::atan2( tg -> get_center_y(), tg -> get_center_x() );
    double comp_eta   = tg -> get_eta();
    double comp_E     = ti -> get_energy();

    const int jeta = m_ihcal_geom -> get_etabin( comp_eta );
    const int jphi = m_ihcal_geom -> get_phibin( comp_phi );
    
    if (jeta < 0 || jphi < 0)
    {
      if (Verbosity() > 3)
      {
        std::cout << "DetermineTowerBackgroundv2::fill_energy_vectors - tower with eta/phi = " << comp_eta << " / " << comp_phi << " is out of bounds, skipping" << std::endl;
      }
      continue;
    }

    if ( src == Jet::SRC::HCALIN_TOWERINFO )
    {
      if ( is_bad || comp_E <= 0.0 )
      {
        m_ihcal_mask.at(jeta).at(jphi) = true;
      }
      m_ihcal_energy.at(jeta).at(jphi) += comp_E;
    }
    if ( src == Jet::SRC::HCALOUT_TOWERINFO )
    {
      if ( is_bad || comp_E <= 0.0 )
      {
        m_ohcal_mask.at(jeta).at(jphi) = true;
      } 
      m_ohcal_energy.at(jeta).at(jphi) += comp_E;
    }
    if ( src == Jet::SRC::CEMC_TOWERINFO  || src == Jet::SRC::CEMC_TOWERINFO_RETOWER )
    {
      if ( is_bad || comp_E <= 0.0 )
      {
        m_emcal_mask.at(jeta).at(jphi) = true;
      }
      m_emcal_energy.at(jeta).at(jphi) += comp_E;
    }
  }

  if (Verbosity() > 2)
  {
    std::cout << "DetermineTowerBackgroundv2::fill_energy_vectors - finished filling energy vectors for src = " << static_cast<int>(src) << std::endl;
    
  }

  return Fun4AllReturnCodes::EVENT_OK;
}

int DetermineTowerBackgroundv2::get_psi2(PHCompositeNode *topNode)
{
  m_psi2 = 0.0;

  if ( m_psi2_mode == 0 )
  {
    m_psi2 = 0.0;
  }
  else if ( m_psi2_mode == 1 )
  {
    double calo_cosine_sum = 0.0;
    double calo_sine_sum   = 0.0;
    for ( int iphi = 0; iphi < m_num_phi_ihcal; iphi++ )
    {
      double en = 0.0;
      int n_towers = 0;
      for ( int ieta = 0; ieta < m_num_eta_ihcal; ieta++ )
      {
        if ( !m_emcal_mask[ieta][iphi] )
        {
          en += m_emcal_energy[ieta][iphi] / m_ue_density.at(0).at(ieta);
          n_towers++;
        }
        if ( !m_ihcal_mask[ieta][iphi] )
        {
          en += m_ihcal_energy[ieta][iphi] / m_ue_density.at(1).at(ieta);
          n_towers++;
        }
        if ( !m_ohcal_mask[ieta][iphi] )
        {
          en += m_ohcal_energy[ieta][iphi] / m_ue_density.at(2).at(ieta);
          n_towers++;
        }
      }

      double phi = m_ihcal_geom -> get_phicenter( iphi );
      if ( n_towers > 0 )
      {
        en /= n_towers;
      }

      calo_cosine_sum += en * cos( 2.0 * phi );
      calo_sine_sum   += en * sin( 2.0 * phi );
    }

    m_psi2 = 0.5 * std::atan2( calo_sine_sum, calo_cosine_sum );
    
    if ( Verbosity() > 0 )
    {
      std::cout << "DetermineTowerBackgroundv2::GetPsi: flow extracted from tower map, setting Psi2 = " << m_psi2 << " ( " << m_psi2 / M_PI << " * pi ) " << std::endl;
    }
  }
  else if ( m_psi2_mode == 2 )
  { 
    auto * eventhead = findNode::getClass<EventHeader>( topNode, "EventHeader" );
    if ( !eventhead ) 
    {
      std::cout << PHWHERE << " Input node EventHeader not found on node tree " << std::endl;
      exit(1);
    }

    m_psi2 = eventhead -> get_FlowPsiN(2);
    if ( Verbosity() > 0 )
    {
      std::cout << "DetermineTowerBackgroundv2::GetPsi: flow extracted from EventHeader, setting Psi2 = " << m_psi2 << " ( " << m_psi2 / M_PI << " * pi ) " << std::endl;
    }

  }
  else if ( m_psi2_mode == 3 )
  { 
    // sEPD event plane extraction
    auto * epmap = findNode::getClass<EventplaneinfoMap>(topNode, "EventplaneinfoMap");
    if (!epmap)
    {
      std::cout << PHWHERE << " Input node EventplaneinfoMap not found on node tree " << std::endl;
      exit(-1);
    }
    if ( !epmap->empty() )
    {
      auto *psin_shifted = epmap->get(EventplaneinfoMap::sEPDNS);
      m_psi2 = psin_shifted->get_shifted_psi(2);
    }
    if (Verbosity() > 0)
    {
      std::cout << "DetermineTowerBackgroundv2::process_event: flow extracted from sEPD, setting Psi2 = " << m_psi2 << " ( " << m_psi2 / M_PI << " * pi ) " << std::endl;
    }

  }
  else
  {
    std::cout << "DetermineTowerBackgroundv2::GetPsi - invalid psi2_mode = " << m_psi2_mode << ", exiting" << std::endl;
    exit(1);
  }
  
  return Fun4AllReturnCodes::EVENT_OK;
}

int DetermineTowerBackgroundv2::get_v2(PHCompositeNode *topNode)
{

  // get v2
  m_v2 = 0.0; // default to 0

  if ( m_flow_mode == 0 )
  {
    m_v2 = 0.0;
  }
  else if ( m_flow_mode == 1)
  {
    if (Verbosity() > 1)
    {
      std::cout << "DetermineTowerBackground::Getv2: flow enabled, calculating flow..." << std::endl;
    }

    double v2_cosine_sum = 0.0;
    double weight_sum   = 0.0; 
    for ( int iphi = 0; iphi < m_num_phi_ihcal; iphi++ )
    {
      
      double en = 0.0;
      int n_towers = 0;
      double phi = m_ihcal_geom -> get_phicenter( iphi );

      for ( int ieta = 0; ieta < m_num_eta_ihcal; ieta++ )
      {
        if ( !m_emcal_mask[ieta][iphi] )
        {
          en += m_emcal_energy[ieta][iphi] / m_ue_density.at(0).at(ieta);
          n_towers++;
        }
        if ( !m_ihcal_mask[ieta][iphi] )
        {
          en += m_ihcal_energy[ieta][iphi] / m_ue_density.at(1).at(ieta);
          n_towers++;
        }
        if ( !m_ohcal_mask[ieta][iphi] )
        {
          en += m_ohcal_energy[ieta][iphi] / m_ue_density.at(2).at(ieta);
          n_towers++;
        }
      }

      if ( n_towers > 0 )
      {
        en /= n_towers;
      }
      
      v2_cosine_sum += en * cos( 2.0 * (phi - m_psi2) );
      weight_sum += en;

    }

    if ( weight_sum != 0.0 )
    {
      m_v2 = v2_cosine_sum / weight_sum;
    }

    if ( Verbosity() > 0 )    
    {
      std::cout << "DetermineTowerBackgroundv2::Getv2: flow calculated from tower map with event plane correction, setting v2 = " << m_v2 << std::endl;
    }

  }  // if do flow
  else if ( m_flow_mode == 2 )
  {
    auto * centinfo = findNode::getClass<CentralityInfo>(topNode, "CentralityInfo");
    if (!centinfo)
    {
      std::cout << "DetermineTowerBackground::process_event: FATAL, CentralityInfo does not exist, cannot extract centrality with do_flow = " << m_flow_mode << std::endl;
      exit(-1);
    }

    int centrality_bin = centinfo->get_centrality_bin(CentralityInfo::PROP::mbd_NS);  
    if (centrality_bin > 0 && centrality_bin < 95) 
    {
	    m_v2 = m_cent_avg_v2[centrality_bin];
	  }
    
    if ( Verbosity() > 0 )
    {
      std::cout << "DetermineTowerBackgroundv2::Getv2: flow extracted from centrality with event plane correction, setting v2 = " << m_v2 << std::endl;
    }
  
  }
  else 
  {
    std::cout << "DetermineTowerBackgroundv2::Getv2 - invalid flow_mode = " << m_flow_mode << ", exiting" << std::endl;
    exit(1);
  }

  if ( Verbosity() > 1 )
  {
    std::cout << "DetermineTowerBackgroundv2::Getv2 - finished calculating v2, v2 = " << m_v2 << std::endl;
  }

  return Fun4AllReturnCodes::EVENT_OK;
}

int DetermineTowerBackgroundv2::init_event(PHCompositeNode *topNode)
{
  if ( first )
  {
    if (Verbosity() > 0)
    {
      std::cout << "DetermineTowerBackgroundv2::init_event - first event, initializing energy vectors and binning" << std::endl;
    }

    auto * rtgc_ihcal = findNode::getClass<RawTowerGeomContainer>(topNode, m_ihcal_geom_node.c_str());
    auto * rtgc_ohcal = findNode::getClass<RawTowerGeomContainer>(topNode, m_ohcal_geom_node.c_str());
    auto * rtgc_cemc  = findNode::getClass<RawTowerGeomContainer>(topNode, m_cemc_geom_node.c_str());

    m_num_eta_ihcal = rtgc_ihcal -> get_etabins();
    m_num_phi_ihcal = rtgc_ihcal -> get_phibins();

    m_z0_bin_edges.resize(m_num_eta_ihcal + 1, 0);
    m_eta_bin_edges.resize(m_num_eta_ihcal + 1, 0);

    const auto dummy_emcal_key = RawTowerDefs::encode_towerid( RawTowerDefs::CalorimeterId::CEMC, 0,0 );
    const auto dummy_ihcal_key = RawTowerDefs::encode_towerid( RawTowerDefs::CalorimeterId::HCALIN, 0,0 );
    const auto dummy_ohcal_key = RawTowerDefs::encode_towerid( RawTowerDefs::CalorimeterId::HCALOUT, 0,0 );

    auto * emcal_dummy_geom = rtgc_cemc -> get_tower_geometry( dummy_emcal_key );
    assert( emcal_dummy_geom );

    auto * ihcal_dummy_geom = rtgc_ihcal -> get_tower_geometry( dummy_ihcal_key );
    assert( ihcal_dummy_geom );

    auto * ohcal_dummy_geom = rtgc_ohcal -> get_tower_geometry( dummy_ohcal_key );
    assert( ohcal_dummy_geom );

    m_ihcal_r = ihcal_dummy_geom -> get_center_radius();
    m_emcal_r = emcal_dummy_geom -> get_center_radius();
    m_ohcal_r = ohcal_dummy_geom -> get_center_radius();
    
    for ( int ieta = 0; ieta < m_num_eta_ihcal; ieta++ )
    {
      auto etabounds = rtgc_ihcal -> get_etabounds(ieta);
      m_z0_bin_edges.at(ieta)    = sinh( etabounds.first ) * m_ihcal_r;
      m_z0_bin_edges.at(ieta+1)  = sinh( etabounds.second ) * m_ihcal_r;
    }
   
    m_emcal_energy.resize(m_num_eta_ihcal, std::vector<float>(m_num_phi_ihcal, 0));
    m_ihcal_energy.resize(m_num_eta_ihcal, std::vector<float>(m_num_phi_ihcal, 0));
    m_ohcal_energy.resize(m_num_eta_ihcal, std::vector<float>(m_num_phi_ihcal, 0));

    m_emcal_mask.resize(m_num_eta_ihcal, std::vector<bool>(m_num_phi_ihcal, false));
    m_ihcal_mask.resize(m_num_eta_ihcal, std::vector<bool>(m_num_phi_ihcal, false));
    m_ohcal_mask.resize(m_num_eta_ihcal, std::vector<bool>(m_num_phi_ihcal, false));

    m_ue_density.resize(3, std::vector<float>(m_num_eta_ihcal, 0));

    first = false;
  }

  m_ue_density.assign(3, std::vector<float>(m_num_eta_ihcal, 0));

  m_emcal_energy.assign(m_num_eta_ihcal, std::vector<float>(m_num_phi_ihcal, 0));
  m_ihcal_energy.assign(m_num_eta_ihcal, std::vector<float>(m_num_phi_ihcal, 0));
  m_ohcal_energy.assign(m_num_eta_ihcal, std::vector<float>(m_num_phi_ihcal, 0));

  m_emcal_mask.assign(m_num_eta_ihcal, std::vector<bool>(m_num_phi_ihcal, false));
  m_ihcal_mask.assign(m_num_eta_ihcal, std::vector<bool>(m_num_phi_ihcal, false));
  m_ohcal_mask.assign(m_num_eta_ihcal, std::vector<bool>(m_num_phi_ihcal, false));

  m_is_flow_failure = false;
  m_psi2 = 0.0;
  m_v2 = 0.0;
  m_vtxz = 0.0;

  m_towerinfos = nullptr; 
  m_towergeom  = nullptr;
  m_caloid = RawTowerDefs::CalorimeterId::NONE;

  get_zvrtx(topNode);

  if ( m_seed_type == 0 )
  {
    for (int i = 0; i < m_num_eta_ihcal; i++)
    {
      m_eta_bin_edges.at(i) = asinh( ( m_z0_bin_edges.at(i) - m_vtxz )/ m_ihcal_r );
      m_eta_bin_edges.at(i+1) = asinh( ( m_z0_bin_edges.at(i+1) - m_vtxz )/ m_ihcal_r );
    }
  }

  return Fun4AllReturnCodes::EVENT_OK;
}

int DetermineTowerBackgroundv2::process_event(PHCompositeNode *topNode)
{
  if ( Verbosity() > 1 )
  {
    std::cout << "DetermineTowerBackgroundv2::process_event - starting process_event" << std::endl;
  }
 
  init_event(topNode);
  
  m_ihcal_geom = LoadTowerGeomContainer(topNode, m_ihcal_geom_node);

  auto * ktjets = findNode::getClass<JetContainer>(topNode, m_seed_jet_name.c_str() );
  if ( !ktjets )
  {
    std::cout << "DetermineTowerBackgroundv2::process_event - cannot find " << m_seed_jet_name << ", exiting" << std::endl;
    exit(1);
  }

  m_idx_seedD    = ktjets -> property_index(Jet::PROPERTY::prop_SeedD);
  m_idx_seed_itr = ktjets -> property_index(Jet::PROPERTY::prop_SeedItr);

  std::vector< std::pair< size_t , double > > seed_val_pairs {};

  for ( size_t i = 0; i < ktjets -> size(); i++ )
  {

    auto this_jet = ktjets -> get_jet(i);
    if ( !this_jet )
    {
      continue;
    }

    double this_val = 0.0;
    auto this_pt  = this_jet -> get_pt();
    
    if ( m_seed_type == 0 )
    {
      this_jet -> set_property( m_idx_seed_itr, 0);
      this_jet -> set_property( m_idx_seedD, 0);
    }

    if ( this_pt < m_seed_jet_pt )
    {
      if ( Verbosity() > 2 )
      {
        std::cout << "DetermineTowerBackgroundv2::process_event: " << \
          "jet with pt / eta / phi = " << this_pt << " / " << this_jet -> get_eta() << " / " << this_jet -> get_phi() << \
          " below seed threshold of " << m_seed_jet_pt << ", skipping" \
        << std::endl;
      }

      this_jet -> set_property( m_idx_seed_itr, 0);
      
      continue;
    }

    if ( m_seed_type == 0 )
    {  
      std::map<int, double> constituent_ETsum {};
      for ( const auto & comp : this_jet -> get_comp_vec() )
      {

        double tower_r = 0.0;
        m_towerinfos = nullptr;
        m_towergeom = nullptr;
        m_caloid = RawTowerDefs::CalorimeterId::NONE;
        if ( comp.first == Jet::SRC::HCALIN_TOWERINFO )
        {
          m_towerinfos = LoadTowerInfoContainer(topNode, m_ihcal_towerinfo_node);
          m_towergeom  = LoadTowerGeomContainer(topNode, m_ihcal_geom_node);
          m_caloid     = RawTowerDefs::CalorimeterId::HCALIN;
          tower_r = m_ihcal_r;
        }
        if ( comp.first == Jet::SRC::HCALOUT_TOWERINFO )
        {
          m_towerinfos  = LoadTowerInfoContainer(topNode, m_ohcal_towerinfo_node);
          m_towergeom   = LoadTowerGeomContainer(topNode, m_ohcal_geom_node);
          m_caloid      = RawTowerDefs::CalorimeterId::HCALOUT;
          tower_r = m_ohcal_r;
        }
        if ( comp.first == Jet::SRC::CEMC_TOWERINFO_RETOWER  )
        {
          m_towerinfos = LoadTowerInfoContainer(topNode, m_cemc_retowerinfo_node);
          m_towergeom  = LoadTowerGeomContainer(topNode, m_ihcal_geom_node);
          m_caloid     = RawTowerDefs::CalorimeterId::HCALIN;
          tower_r = m_emcal_r;
        }
        if ( comp.first == Jet::SRC::CEMC_TOWERINFO  )
        {
          m_towerinfos = LoadTowerInfoContainer(topNode, m_cemc_towerinfo_node);
          m_towergeom  = LoadTowerGeomContainer(topNode, m_cemc_geom_node);
          m_caloid     = RawTowerDefs::CalorimeterId::CEMC;
          tower_r = m_emcal_r;
        }

        auto * ti = m_towerinfos -> get_tower_at_channel( comp.second );
        assert(ti);
        if ( !ti -> get_isGood() )
        {
          if ( Verbosity() > 5 )
          {
            std::cout << "DetermineTowerBackgroundv2::process_event: --> --> Skipping constituent in layer " << comp.first << " at channel " << comp.second << " due to masking" << std::endl;
          }
          continue;
        }

        auto tkey       = m_towerinfos -> encode_key( comp.second );
        int comp_ieta   = m_towerinfos -> getTowerEtaBin( tkey );
        int comp_iphi   = m_towerinfos -> getTowerPhiBin( tkey );
        const auto key  = RawTowerDefs::encode_towerid( m_caloid, comp_ieta, comp_iphi );
        
        auto * tg       = m_towergeom -> get_tower_geometry( key );
        assert(tg);
  
        double tower_z0   = sinh( tg -> get_eta() ) * tower_r;
        double comp_z     = tower_z0 - m_vtxz;
        double comp_eta   = asinh( comp_z / tower_r );
        double comp_phi   = atan2( tg -> get_center_y(), tg -> get_center_x() );
        double comp_eT    = ti -> get_energy() / cosh( comp_eta );
        
        if ( Verbosity() > 4 )
        {
          std::cout << "DetermineTowerBackgroundv2::process_event: --> --> constituent in layer " << comp.first << " at ieta / iphi = " << comp_ieta << " / " << comp_iphi << ", filling map with key = " << key << " and ET = " << comp_eT << std::endl;
        }

        int jeta = -1;
        int jphi = m_ihcal_geom -> get_phibin( comp_phi );
        for ( int ieta = 0; ieta < m_num_eta_ihcal; ieta++ )
        {
          if ( comp_eta >= m_eta_bin_edges.at(ieta) && comp_eta < m_eta_bin_edges.at(ieta+1) )
          {
            jeta = ieta;
            break;
          }
        }
        
        if ( jeta == -1 || jphi == -1 )
        {
          if ( Verbosity() > 1 )
          {
            std::cout << "DetermineTowerBackgroundv2::process_event: --> --> Warning: could not find ieta / iphi bin for constituent in layer " << comp.first << " at ieta / iphi = " << comp_eta << " / " << comp_phi << std::endl;
            static bool once = true;
            if ( once )          {
              once = false;
            
              std::cout << "DetermineTowerBackgroundv2::process_event: --> --> This warning will only be printed once." << std::endl;
              for ( int ieta = 0; ieta < m_num_eta_ihcal + 1; ieta++ )
              {
                std::cout << "DetermineTowerBackgroundv2::process_event: --> --> eta bin edge " << ieta << " is at " << m_eta_bin_edges.at(ieta) << std::endl;
              }
            
            }
          }
          continue;
        }

        int ckey = ( jeta * m_num_phi_ihcal ) + jphi;
        constituent_ETsum[ckey] += comp_eT;
        
        if ( Verbosity() > 4 )
        {
          std::cout << "DetermineTowerBackgroundv2::process_event: --> --> constituent in layer " << comp.first << " at ieta / iphi = " << comp_eta << " / " << comp_phi << ", filling map with key = " << ckey << " and ET = " << comp_eT << std::endl;
          std::cout << "DetermineTowerBackgroundv2::process_event: --> --> map at key = " << ckey << " now has ET = " << constituent_ETsum[ckey] << std::endl;
        }

      }

      double max_supercomp_ET = 0.0, sum_supercomp_ET = 0.0;
      int n_supercomps = 0;
      for ( const auto & map_iter : constituent_ETsum )
      {
        if ( Verbosity() > 4 )
        {
          std::cout << "DetermineTowerBackgroundv2::process_event: --> --> map has key # " << map_iter.first << " and ET = " << map_iter.second << std::endl;
        }
        n_supercomps++;
        sum_supercomp_ET += map_iter.second;
        max_supercomp_ET = std::max<double>( map_iter.second, max_supercomp_ET );
      }

      float mean_supercomp_ET = sum_supercomp_ET / n_supercomps;
      float seed_D = max_supercomp_ET / mean_supercomp_ET;
     
      this_jet -> set_property( m_idx_seedD, seed_D );
      if ( Verbosity() > 3 )
      {
        std::cout << "DetermineTowerBackgroundv2::process_event: --> jet has < ET > = " << sum_supercomp_ET << " / " << n_supercomps << " = " << mean_supercomp_ET << ", max-ET = " << max_supercomp_ET << ", and D = " << seed_D << std::endl;
      }

      this_val = seed_D;
    }

    if ( m_seed_type == 1 )
    {
      this_val = this_pt;
      if ( Verbosity() > 2 )
      {
        std::cout << "DetermineTowerBackgroundv2::process_event: possible seed" << \
          " jet with pt / eta / phi = " << this_pt << " / " << this_jet -> get_eta() << " / " << this_jet -> get_phi() << \
          ", adding to list for seed selection with pT threshold of " << m_seed_jet_pt \
        << std::endl;
      }
    }
    
    if ( Verbosity() > 2 )
    {
      std::cout << "DetermineTowerBackgroundv2::process_event: --> jet has value = " << this_val << std::endl;
    }

    // add to list for seed selection
    seed_val_pairs.push_back( std::make_pair(i, this_val) );
  
  }

  // sort jets by value
  std::sort( seed_val_pairs.begin(), seed_val_pairs.end(), 
    []( const std::pair<size_t, double> & a, const std::pair<size_t, double> & b ) { 
    return a.second > b.second; 
  } );

  // only accept up to m_n_omit_seeds seeds, starting from the highest value, as long as they are above the pt threshold
  std::vector<size_t> accepted_seed_indices {};
  for ( size_t i = 0; i < (size_t)m_n_omit_seeds; i++ )
  {
    if ( i >= seed_val_pairs.size() )
    {
      continue;
    }
    accepted_seed_indices.push_back( seed_val_pairs.at(i).first );
  }
 
  int n_accepted_seeds = 0;
  for ( const auto & i : accepted_seed_indices )
  {

    auto this_jet = ktjets -> get_jet( i );

    assert(this_jet);
  
    double val = 0; 
    for ( const auto & pair : seed_val_pairs )
    {
      if ( pair.first == i )
      {
        val = pair.second;
        break;
      }
    }
    n_accepted_seeds++;
  
    if ( m_seed_type == 0 )
    {
      if ( Verbosity() > 1 )
      {
        std::cout << "DetermineTowerBackgroundv2::process_event: --> omitting jet with pt / eta / phi = " << this_jet -> get_pt() << " / " << this_jet -> get_eta() << " / " << this_jet -> get_phi() << " from seed consideration since it is among the " << m_n_omit_seeds << " jets with highest D value " << val << std::endl;
      }
      this_jet -> set_property( m_idx_seed_itr, 1.0 );
    }
    if ( m_seed_type == 1 )
    {
      if ( Verbosity() > 1 )
      {
        std::cout << "DetermineTowerBackgroundv2::process_event: --> omitting jet with pt / eta / phi = " << this_jet -> get_pt() << " / " << this_jet -> get_eta() << " / " << this_jet -> get_phi() << " from seed consideration since it is among the " << m_n_omit_seeds << " jets with highest pT value " << val << std::endl;
      }
      this_jet -> set_property( m_idx_seed_itr, 2.0 );
    }
    if ( Verbosity() > 1 )
    {
      std::cout << "DetermineTowerBackgroundv2::process_event: --> accepted seed jet with pt / eta / phi = " << this_jet -> get_pt() << " / " << this_jet -> get_eta() << " / " << this_jet -> get_phi() << " and value = " << i << "( nAcc/nOmitted = " << n_accepted_seeds << " / " << m_n_omit_seeds << " )" << std::endl;
    }
    
    for ( const auto & comp : this_jet -> get_comp_vec() )
    {

      double tower_r = 0.0;
      if ( comp.first == Jet::SRC::HCALIN_TOWERINFO )
      {
        m_towerinfos = LoadTowerInfoContainer(topNode, m_ihcal_towerinfo_node);
        m_towergeom  = LoadTowerGeomContainer(topNode, m_ihcal_geom_node);
        m_caloid     = RawTowerDefs::CalorimeterId::HCALIN;
        tower_r = m_ihcal_r;
      }
      if ( comp.first == Jet::SRC::HCALOUT_TOWERINFO )
      {
        m_towerinfos  = LoadTowerInfoContainer(topNode, m_ohcal_towerinfo_node);
        m_towergeom   = LoadTowerGeomContainer(topNode, m_ohcal_geom_node);
        m_caloid      = RawTowerDefs::CalorimeterId::HCALOUT;
        tower_r = m_ohcal_r;
      }
      if ( comp.first == Jet::SRC::CEMC_TOWERINFO_RETOWER  )
      {
        m_towerinfos = LoadTowerInfoContainer(topNode, m_cemc_retowerinfo_node);
        m_towergeom  = LoadTowerGeomContainer(topNode, m_ihcal_geom_node);
        m_caloid     = RawTowerDefs::CalorimeterId::HCALIN;
        tower_r = m_emcal_r;
      }
      if ( comp.first == Jet::SRC::CEMC_TOWERINFO  )
      {
        m_towerinfos = LoadTowerInfoContainer(topNode, m_cemc_towerinfo_node);
        m_towergeom  = LoadTowerGeomContainer(topNode, m_cemc_geom_node);
        m_caloid     = RawTowerDefs::CalorimeterId::CEMC;
        tower_r = m_emcal_r;
      }
      if ( Verbosity() > 4 )
      {
        std::cout << "DetermineTowerBackgroundv2::process_event: --> --> processing constituent in layer " << comp.first << " at channel " << comp.second << " tower_r = " << tower_r << std::endl;
      }

      auto * ti = m_towerinfos -> get_tower_at_channel( comp.second );
      assert(ti);
      // bool is_bad = !ti -> get_isGood();
      auto tkey       = m_towerinfos -> encode_key( comp.second );
      int comp_ieta   = m_towerinfos -> getTowerEtaBin( tkey );
      int comp_iphi   = m_towerinfos -> getTowerPhiBin( tkey );
      const auto key  = RawTowerDefs::encode_towerid( m_caloid, comp_ieta, comp_iphi );
      auto * tg       = m_towergeom -> get_tower_geometry( key );

      assert(tg);
      int jeta = m_ihcal_geom -> get_etabin( tg -> get_eta() );
      int jphi = m_ihcal_geom -> get_phibin( atan2( tg -> get_center_y(), tg -> get_center_x() ) );
      if ( comp.first == Jet::SRC::HCALIN_TOWERINFO )
      {
        m_ihcal_mask[jeta][jphi] = true;
      }
      if ( comp.first == Jet::SRC::HCALOUT_TOWERINFO )
      {
        m_ohcal_mask[jeta][jphi] = true;
      }
      if ( comp.first == Jet::SRC::CEMC_TOWERINFO_RETOWER || comp.first == Jet::SRC::CEMC_TOWERINFO )
      {
        m_emcal_mask[jeta][jphi] = true;
      }
      if ( Verbosity() > 4 )
      {
        std::cout << "DetermineTowerBackgroundv2::process_event: --> --> marking tower in layer " << comp.first << " at ieta / iphi = " << comp_ieta << " / " << comp_iphi << " ( jeta / jphi = " << jeta << " / " << jphi << " ) as masked since it is a constituent of an accepted seed jet" << std::endl;
      }
     
    }

  } 

  // fill energy vectors for non-seed jets
  fill_energy_vectors( topNode, Jet::SRC::HCALIN_TOWERINFO );
  fill_energy_vectors( topNode, Jet::SRC::HCALOUT_TOWERINFO );
  fill_energy_vectors( topNode, Jet::SRC::CEMC_TOWERINFO_RETOWER );

  // set averages
  for (int ieta = 0; ieta < m_num_eta_ihcal; ieta++ )
  {
    float avg_emcal_energy = 0.0;
    float avg_ihcal_energy = 0.0;
    float avg_ohcal_energy = 0.0;
    int n_emcal_towers = 0;
    int n_ihcal_towers = 0;
    int n_ohcal_towers = 0;
    for ( int iphi = 0; iphi < m_num_phi_ihcal; iphi++ )
    { 
      if ( !m_emcal_mask[ieta][iphi] )
      {
        avg_emcal_energy += m_emcal_energy[ieta][iphi];
        n_emcal_towers++;
      }
      if ( !m_ihcal_mask[ieta][iphi] )
      {
        avg_ihcal_energy += m_ihcal_energy[ieta][iphi];
        n_ihcal_towers++;
      }
      if ( !m_ohcal_mask[ieta][iphi] )
      {
        avg_ohcal_energy += m_ohcal_energy[ieta][iphi];
        n_ohcal_towers++;
      }
    }
    if ( n_emcal_towers > 0 )
    {
      avg_emcal_energy /= n_emcal_towers;
    }
    if ( n_ihcal_towers > 0 )
    {
      avg_ihcal_energy /= n_ihcal_towers;
    }
    if ( n_ohcal_towers > 0 )
    {
      avg_ohcal_energy /= n_ohcal_towers;
    }

    if ( Verbosity() > 2 )
    {
      std::cout << "DetermineTowerBackgroundv2::process_event: for eta bin " << ieta << ", avg energy in emcal / ihcal / ohcal is " << avg_emcal_energy << " / " << avg_ihcal_energy << " / " << avg_ohcal_energy << " calculated from " << n_emcal_towers << " / " << n_ihcal_towers << " / " << n_ohcal_towers << " towers" << std::endl;
    }

    m_ue_density.at(0).at(ieta) = avg_emcal_energy;
    m_ue_density.at(1).at(ieta) = avg_ihcal_energy;
    m_ue_density.at(2).at(ieta) = avg_ohcal_energy;
  }

  // Get psi
  get_psi2(topNode);

  if ( !std::isfinite(m_psi2) )
  {
    if (Verbosity() > 0)
    {
      std::cout << "DetermineTowerBackgroundv2::GetPsi - WARNING Psi2 is non-finite (NaN or Inf), setting Psi2 = 0." << std::endl;
    }
    m_is_flow_failure = true;
  }
  
  // Get v2
  get_v2(topNode);

  if ( !std::isfinite(m_v2) || std::abs(m_v2) > 0.5 )
  {
    if (Verbosity() > 0)
    {
      std::cout << "DetermineTowerBackgroundv2::Getv2 - WARNING v2 is non-finite (NaN or Inf), setting v2 = 0 and flow failure flag to true." << std::endl;
    }
    m_is_flow_failure = true;
  }

  if ( m_is_flow_failure )
  {
    m_v2 = 0.0;
    m_psi2 = 0.0;
    if ( Verbosity() > 0 )
    {
      std::cout << "DetermineTowerBackgroundv2::process_event: flow failure detected, setting v2 to 0 and proceeding with background calculation without flow modulation" << std::endl;
    }
  }

  
  for (int ieta = 0; ieta < m_num_eta_ihcal; ieta++ )
  {
    float total_emcal_energy = 0.0;
    float total_ihcal_energy = 0.0;
    float total_ohcal_energy = 0.0;
    int n_emcal_towers = 0;
    int n_ihcal_towers = 0;
    int n_ohcal_towers = 0;
    for ( int iphi = 0; iphi < m_num_phi_ihcal; iphi++ )
    {
      double phi = m_ihcal_geom -> get_phicenter( iphi );
      double modulation = 1.0 + 2.0 * m_v2 * cos( 2.0 * ( phi - m_psi2 ) );
      if ( !m_emcal_mask[ieta][iphi] )
      {
        total_emcal_energy += m_emcal_energy[ieta][iphi] / modulation;
        n_emcal_towers++;
      }
      if ( !m_ihcal_mask[ieta][iphi] )
      {
        total_ihcal_energy += m_ihcal_energy[ieta][iphi] / modulation;
        n_ihcal_towers++;
      }
      if ( !m_ohcal_mask[ieta][iphi] )
      {
        total_ohcal_energy += m_ohcal_energy[ieta][iphi] / modulation;
        n_ohcal_towers++;
      }
      
    } // end loop over phi bins


    if ( n_emcal_towers > 0 )
    {
      m_ue_density.at(0).at(ieta) = total_emcal_energy / n_emcal_towers;
    }
    if ( n_ihcal_towers > 0 )
    {
      m_ue_density.at(1).at(ieta) = total_ihcal_energy / n_ihcal_towers;
    }
    if ( n_ohcal_towers > 0 )
    {
      m_ue_density.at(2).at(ieta) = total_ohcal_energy / n_ohcal_towers;
    }

    m_ntowers += n_emcal_towers + n_ihcal_towers + n_ohcal_towers;
    
    if ( n_emcal_towers + n_ihcal_towers + n_ohcal_towers > 0 )
    {
      m_nstrips++;
    }
    
    if ( Verbosity() > 0 )    
    {
      std::cout << "DetermineTowerBackgroundv2::process_event: after flow modulation, average energy in eta strip " << ieta << " is " << m_ue_density.at(0).at(ieta) << " for EMCAL, " << m_ue_density.at(1).at(ieta) << " for IHCAL, and " << m_ue_density.at(2).at(ieta) << std::endl;
    }

  }


  // check for any negative UE , or extremely high UE that might indicate a problem
  for (int ieta = 0; ieta < m_num_eta_ihcal; ieta++ )
  {
    if ( m_ue_density.at(0).at(ieta)   < 0 
        || m_ue_density.at(1).at(ieta) < 0 
        || m_ue_density.at(2).at(ieta) < 0 )
    {
      if ( Verbosity() > 0 )
      {
        std::cout << "DetermineTowerBackgroundv2::process_event: WARNING: negative UE density detected in eta bin " << ieta << ", setting flow failure flag to true and UE density to 0" << std::endl;
      }
      m_ue_density.at(0).at(ieta) = std::max( 0.0f, m_ue_density.at(0).at(ieta) );
      m_ue_density.at(1).at(ieta) = std::max( 0.0f, m_ue_density.at(1).at(ieta) );
      m_ue_density.at(2).at(ieta) = std::max( 0.0f, m_ue_density.at(2).at(ieta) );
    }
  }


  if ( Verbosity() > 0 )
  {
    std::cout << "DetermineTowerBackgroundv2::process_event: done calculating background, EMCal UE density = [ ";
    for (int ieta = 0; ieta < m_num_eta_ihcal; ieta++ )
    {
      std::cout << m_ue_density.at(0).at(ieta) << " ";
    }
    std::cout << "],\n IHCAL UE density = [ ";
    for (int ieta = 0; ieta < m_num_eta_ihcal; ieta++ )
    {
      std::cout << m_ue_density.at(1).at(ieta) << " ";
    }
    std::cout << "],\n OHCAL UE density = [ ";
    for (int ieta = 0; ieta < m_num_eta_ihcal; ieta++ )
    {
      std::cout << m_ue_density.at(2).at(ieta) << " ";
    }
    std::cout << "]" << std::endl;
  }
    
  FillNode( topNode , m_background_node );

  if ( Verbosity() > 0 )
  {
    std::cout << "DetermineTowerBackgroundv2::process_event: filled TowerBackground node with background information" << std::endl;
  }


  return Fun4AllReturnCodes::EVENT_OK;
}

int DetermineTowerBackgroundv2::CreateNode(PHCompositeNode *topNode)
{
  PHNodeIterator iter(topNode);

  // Looking for the DST node
  PHCompositeNode *dstNode = dynamic_cast<PHCompositeNode *>(iter.findFirst("PHCompositeNode", "DST"));
  if (!dstNode)
  {
    std::cout << PHWHERE << "DST Node missing, doing nothing." << std::endl;
    return Fun4AllReturnCodes::ABORTRUN;
  }

  // store the jet background stuff under a sub-node directory
  PHCompositeNode *bkgNode = dynamic_cast<PHCompositeNode *>(iter.findFirst("PHCompositeNode", "JETBACKGROUND"));
  if (!bkgNode)
  {
    bkgNode = new PHCompositeNode("JETBACKGROUND");
    dstNode->addNode(bkgNode);
  }

  // create the TowerBackground node...
  TowerBackground *towerbackground = findNode::getClass<TowerBackground>(topNode, m_background_node);
  if (!towerbackground)
  {
    towerbackground = new TowerBackgroundv1();
    PHIODataNode<PHObject> *bkgDataNode = new PHIODataNode<PHObject>(towerbackground, m_background_node, "PHObject");
    bkgNode->addNode(bkgDataNode);
  }
  else
  {
    std::cout << PHWHERE << "::ERROR - " << m_background_node << " pre-exists, but should not" << std::endl;
    // exit(-1);
  }

  return Fun4AllReturnCodes::EVENT_OK;
}

void DetermineTowerBackgroundv2::FillNode(PHCompositeNode *topNode , const std::string & name)
{
  auto * towerbackground = findNode::getClass<TowerBackground>(topNode, name);
  if (!towerbackground)
  {
    std::cout << " ERROR -- can't find TowerBackground node after it should have been created" << std::endl;
    return;
  }

  towerbackground->set_UE(0, m_ue_density.at(0) );
  towerbackground->set_UE(1, m_ue_density.at(1) );
  towerbackground->set_UE(2, m_ue_density.at(2) );

  towerbackground->set_v2(m_v2);

  towerbackground->set_Psi2(m_psi2);

  towerbackground->set_nStripsUsedForFlow(m_nstrips);

  towerbackground->set_nTowersUsedForBkg(m_ntowers);

  towerbackground->set_flow_failure_flag(m_is_flow_failure);

  return;
}





