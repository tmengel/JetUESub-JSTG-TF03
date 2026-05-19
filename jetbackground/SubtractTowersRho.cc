#include "SubtractTowersRho.h"

#include "TowerRho.h"
#include "TowerRhov1.h"

// sPHENIX includes
#include <calobase/RawTower.h>
#include <calobase/RawTowerContainer.h>
#include <calobase/RawTowerDefs.h>
#include <calobase/RawTowerGeom.h>
#include <calobase/RawTowerGeomContainer.h>
#include <calobase/RawTowerv1.h>
#include <calobase/RawTowerDefs.h>

#include <globalvertex/GlobalVertexv3.h>
#include <globalvertex/GlobalVertexMapv1.h>

#include <calobase/TowerInfo.h>
#include <calobase/TowerInfoContainer.h>

#include <fun4all/Fun4AllReturnCodes.h>
#include <fun4all/SubsysReco.h>

#include <phool/PHCompositeNode.h>
#include <phool/PHIODataNode.h>
#include <phool/PHNode.h>
#include <phool/PHNodeIterator.h>
#include <phool/PHObject.h>
#include <phool/getClass.h>
#include <phool/phool.h>

#include <TMath.h>

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <map>
#include <utility>
#include <vector>
#include <cassert>

int SubtractTowersRho::InitRun(PHCompositeNode *topNode)
{
  return CreateNode(topNode);
}

int SubtractTowersRho::grab_zvrtx( PHCompositeNode *topNode )
{
  if (Verbosity() > 0)
  {
    std::cout << "SubtractTowersRho::grab_zvrtx - starting grab_zvrtx with m_vertex_type = " << m_vertex_type << std::endl;
  }

  m_vtxz = 0;  // default to 0
  auto vertexmap = findNode::getClass< GlobalVertexMap >( topNode, "GlobalVertexMap" );
  if ( !vertexmap )
  {
    std::cout << "SubtractTowersRho::grab_zvrtx - Fatal Error - GlobalVertexMap node is missing. Please turn on the do_global flag in the main macro in order to reconstruct the global vertex." << std::endl;
    return Fun4AllReturnCodes::ABORTEVENT;
  }

  if ( vertexmap->empty() )
  {
    if (Verbosity() > 0 )
    {
      std::cout << "SubtractTowersRho::grab_zvrtx - empty vertex map, continuing as if zvtx = 0" << std::endl;
    }
    m_vtxz = 0;  // default to 0
    return Fun4AllReturnCodes::EVENT_OK;
  }

  if ( m_vertex_type == GlobalVertex::UNDEFINED )
  {
    auto * vtx = vertexmap->begin()->second;
    if ( vtx )
    {
      m_vtxz = vtx->get_z();
    }
  }
  else
  {
    auto vertices = vertexmap -> get_gvtxs_with_type( { m_vertex_type } );
    if( !vertices.empty() && vertices.at(0) )
    {
      m_vtxz = vertices.at(0) -> get_z();
    }
  }

  if ( std::isnan(m_vtxz)  || std::abs(m_vtxz) > 1e3 )
  {

    static bool once = true;
    if (once)
    {
      once = false;
      std::cout << "SubtractTowersRho::grab_zvrtx - WARNING - vertex is " << m_vtxz << ". Continue with zvtx = 0 (further vertex warning will be suppressed)." << std::endl;
    }
    m_vtxz = 0;
  }

  if (Verbosity() > 1)
  {
    std::cout << "SubtractTowersRho::grab_zvrtx - finished grab_zvrtx with m_vtxz = " << m_vtxz << std::endl;
  }
  return Fun4AllReturnCodes::EVENT_OK;
  
  

}

int SubtractTowersRho::process_event(PHCompositeNode *topNode)
{

  if (Verbosity() > 1)
  {
    std::cout << "SubtractTowersRho::process_event - starting event processing" << std::endl;
  }

  auto * rho_node = findNode::getClass<TowerRhov1 >(topNode, m_rhoNode);
  if ( !rho_node )
  {
    std::cout << PHWHERE << "TowerRho node " << m_rhoNode << " not found, doing nothing." << std::endl;
    return Fun4AllReturnCodes::ABORTEVENT;
  }

  auto res = grab_zvrtx( topNode );
  if ( res != Fun4AllReturnCodes::EVENT_OK )
  {
    if ( Verbosity() > 0 )    
    {
      std::cout << "SubtractTowersRho::process_event - grab_zvrtx failed, skipping tower subtraction for this event" << std::endl;
    }
    return res;
  }

  auto rho_val = rho_node -> get_rho();
  const double MULT_THRES_VAL = TMath::Sqrt(2 * 1.0 );
  for ( const auto & target_node_name : m_targetTowerNodes )
  {

    auto * towerinfos = findNode::getClass<TowerInfoContainer>(topNode, target_node_name.c_str());
    if ( !towerinfos )
    {
      std::cout << PHWHERE << "TowerInfo node " << target_node_name << " not found, doing nothing." << std::endl;
      return Fun4AllReturnCodes::ABORTEVENT;
    }
    auto output_name = get_outputTowerNode(target_node_name, m_subSuffix);

    auto * sub_towerinfos = findNode::getClass<TowerInfoContainer>(topNode, output_name.c_str());
    if ( !sub_towerinfos )
    {
      std::cout << PHWHERE << "Subtracted TowerInfo node " << output_name << " not found, doing nothing." << std::endl;
      return Fun4AllReturnCodes::ABORTEVENT;
    }

    bool is_emcal = (target_node_name.find("CEMC") != std::string::npos);
    bool is_ihcal = (target_node_name.find("HCALIN") != std::string::npos);
    bool is_ohcal = (target_node_name.find("HCALOUT") != std::string::npos);
    bool is_retowered = (target_node_name.find("RETOWER") != std::string::npos);
    
    std::string geo_node_name = is_emcal ? (is_retowered ? "TOWERGEOM_HCALIN" : "TOWERGEOM_CEMC") : (is_ihcal ? "TOWERGEOM_HCALIN" : (is_ohcal ? "TOWERGEOM_HCALOUT" : ""));
    RawTowerDefs::CalorimeterId calo_id = is_emcal ? ( is_retowered ? RawTowerDefs::CalorimeterId::HCALIN : RawTowerDefs::CalorimeterId::CEMC ) : (is_ihcal ? RawTowerDefs::CalorimeterId::HCALIN :  (is_ohcal ? RawTowerDefs::CalorimeterId::HCALOUT : RawTowerDefs::CalorimeterId::NONE) );

    auto * geom = findNode::getClass<RawTowerGeomContainer>(topNode, geo_node_name.c_str());
    if ( !geom )   
    {
      std::cout << PHWHERE << "RawTowerGeomContainer node " << geo_node_name << " not found, doing nothing." << std::endl;
      return Fun4AllReturnCodes::ABORTEVENT;
    }
  
    auto n_eta = geom -> get_etabins();
    std::vector< double > dA_vals (n_eta, 0);
    for (auto ieta = 0; ieta < n_eta; ++ieta)
    {
      auto etabounds = geom -> get_etabounds(ieta);
      auto phibounds = geom -> get_phibounds(0);  // assume constant phi binning
      double dphi = phibounds.second - phibounds.first;
      double deta = etabounds.second - etabounds.first;
      dA_vals.at(ieta) = deta * dphi;
    }
    
    double calo_radius = 0;
    if ( is_emcal && is_retowered )
    {
      // this needs to load the emcal geo 
      const RawTowerDefs::keytype EMCal_key = RawTowerDefs::encode_towerid(RawTowerDefs::CalorimeterId::CEMC, 0, 0);
      auto emcal_geom = findNode::getClass<RawTowerGeomContainer>(topNode, "TOWERGEOM_CEMC");
      if ( !emcal_geom )
      {
        std::cout << PHWHERE << "RawTowerGeomContainer node TOWERGEOM_CEMC not found, cannot determine calo radius for flow modulation, doing nothing." << std::endl;
        return Fun4AllReturnCodes::ABORTEVENT;
      }
      auto emcal_tower_geom = emcal_geom -> get_tower_geometry(EMCal_key);
      assert(emcal_tower_geom);
      calo_radius = emcal_tower_geom -> get_center_radius();
    }
    else
    {
      const RawTowerDefs::keytype key = RawTowerDefs::encode_towerid(calo_id, 0, 0);
      auto tower_geom = geom -> get_tower_geometry(key);
      assert(tower_geom);
      calo_radius = tower_geom -> get_center_radius();
    }

    for ( auto ich = 0; ich < (int)towerinfos -> size(); ++ich)
    {
      auto tower = towerinfos -> get_tower_at_channel(ich);
      assert(tower);

      auto tower_key = towerinfos -> encode_key(ich);
      auto ieta = towerinfos -> getTowerEtaBin(tower_key);
      auto iphi = towerinfos -> getTowerPhiBin(tower_key);
      const RawTowerDefs::keytype geo_key = RawTowerDefs::encode_towerid(calo_id, ieta, iphi);
      
      auto raw_E = tower -> get_energy();
      if ( !tower -> get_isGood() || std::isnan(raw_E) )
      {
        // don't waste time
        sub_towerinfos -> get_tower_at_channel(ich) -> set_energy(0);
        sub_towerinfos -> get_tower_at_channel(ich) -> set_time(tower -> get_time());
        sub_towerinfos -> get_tower_at_channel(ich) -> set_status(tower -> get_status());
        continue;
      } 

      auto tower_geom = geom -> get_tower_geometry(geo_key);
      assert(tower_geom);
      
      auto eta0 = tower_geom -> get_eta();
      auto z0 =  sinh(eta0) * calo_radius;
      auto dz = z0 - m_vtxz;
      double eta = asinh( dz / calo_radius);
      double UE = rho_val * cosh(eta) ;
      if ( m_rho_method == TowerRho::MULT )
      {
        double eT = raw_E / cosh(eta);
        if ( eT > MULT_THRES_VAL * rho_val  )
        {
          UE = 0;
        }
      }
      else if ( m_rho_method == TowerRho::AREA )
      {
        UE *= dA_vals.at(ieta);
      }

      double sub_E = raw_E - UE;
      
      sub_towerinfos -> get_tower_at_channel(ich) -> set_energy(sub_E);
      sub_towerinfos -> get_tower_at_channel(ich) -> set_time(tower -> get_time());
      sub_towerinfos -> get_tower_at_channel(ich) -> set_status(tower -> get_status());
      if (Verbosity() > 5)
      {
        std::cout << "SubtractTowersRho::process_event - tower " << ich << " at ieta / iphi = " << ieta << " / " << iphi << ", raw E = " << raw_E << ", UE = " << UE << ", subtracted E = " << sub_E << std::endl;
      }
    }
    
    if (Verbosity() > 1)
    {
      std::cout << "SubtractTowersRho::process_event - finished processing target node " << target_node_name << std::endl;
    }

  }

  if (Verbosity() > 1)
  {
    std::cout << "SubtractTowersRho::process_event - finished event processing" << std::endl;
  }
  return Fun4AllReturnCodes::EVENT_OK;

}

int SubtractTowersRho::CreateNode( PHCompositeNode *topNode )
{
  PHNodeIterator iter(topNode);

  // Looking for the DST node
  auto dstNode = dynamic_cast<PHCompositeNode *>(iter.findFirst("PHCompositeNode", "DST"));
  if ( !dstNode )
  {
    std::cout << PHWHERE << "DST Node missing, doing nothing." << std::endl;
    return Fun4AllReturnCodes::ABORTRUN;
  }

  auto * rho_node = findNode::getClass<TowerRhov1 >(topNode, m_rhoNode);
  if ( !rho_node )
  {
    std::cout << PHWHERE << "TowerRho node " << m_rhoNode << " not found, doing nothing." << std::endl;
    return Fun4AllReturnCodes::ABORTRUN;
  }

  if ( m_subSuffix.empty() )
  {
    m_subSuffix = rho_node -> get_method_string(rho_node -> get_method());
  }

  m_rho_method = rho_node -> get_method ();

  if (Verbosity() > 0)
  {
    std::cout << "SubtractTowersRho::CreateNode - using rho method " << m_rho_method << " with suffix " << m_subSuffix << std::endl;
  }

  // loop over target nodes and create new tower nodes for the subtracted towers, if they don't already exist
  for (const auto & target_node_name : m_targetTowerNodes)
  {

    bool is_emcal = (target_node_name.find("CEMC") != std::string::npos);
    bool is_ihcal = (target_node_name.find("HCALIN") != std::string::npos);
    bool is_ohcal = (target_node_name.find("HCALOUT") != std::string::npos);

    std::string target_node_query_str = is_emcal ? "CEMC" : (is_ihcal ? "HCALIN" : (is_ohcal ? "HCALOUT" : ""));
    if ( target_node_query_str.empty() )
    {
      std::cout << PHWHERE << "Target node name " << target_node_name << " does not match expected calorimeter types, skipping." << std::endl;
      continue;
    }

    auto * towerinfos = findNode::getClass<TowerInfoContainer>(topNode, target_node_name.c_str());
    if ( !towerinfos )
    {
      std::cout << PHWHERE << "TowerInfo node " << target_node_name << " not found, doing nothing." << std::endl;
      continue;
    }
    
    auto * calonode = dynamic_cast<PHCompositeNode *>(iter.findFirst("PHCompositeNode", target_node_query_str.c_str()));
    if ( !calonode )
    {
      std::cout << PHWHERE << "Calorimeter Node " << target_node_query_str << " not found, doing nothing." << std::endl;
      continue;
    }

    auto output_name = get_outputTowerNode(target_node_name, m_subSuffix);

    auto * test_towerinfos = findNode::getClass<TowerInfoContainer>(topNode, output_name.c_str());
    if ( !test_towerinfos )
    {
      if (Verbosity() > 0)
      {
        std::cout << "SubtractTowersRho::CreateNode : creating " << output_name << " node " << std::endl;
      }
      auto * new_towerinfos =  dynamic_cast<TowerInfoContainer *>( towerinfos->CloneMe() );
      auto * towerInfoNode = new PHIODataNode<PHObject>(new_towerinfos, output_name.c_str(), "PHObject");
      calonode -> addNode( towerInfoNode );
    }
    else
    {
      std::cout << "SubtractTowersRho::CreateNode : " << output_name << " already exists! " << std::endl;
    }
  }

  if (Verbosity() > 0)
  {
    std::cout << "SubtractTowersRho::CreateNode - finished creating nodes" << std::endl;
  }

  return Fun4AllReturnCodes::EVENT_OK;
}
