#ifndef JETBACKGROUND_SUBTRACTTOWERSRHO_H
#define JETBACKGROUND_SUBTRACTTOWERSRHO_H

#include <fun4all/SubsysReco.h>

#include "TowerRho.h"
#include <globalvertex/GlobalVertex.h>

#include <string>

// forward declarations
class PHCompositeNode;

/// \class SubtractTowersRho
///
/// \brief creates new UE-subtracted towers
class SubtractTowersRho : public SubsysReco
{
 public:
  SubtractTowersRho(const std::string &name = "SubtractTowersRho") : SubsysReco(name) {}
  ~SubtractTowersRho() override {}

  int InitRun(PHCompositeNode *topNode) override;
  int process_event(PHCompositeNode *topNode) override;

  void set_flowMod( const bool b ) { m_doFlowMod = b; }
  void set_rhoNode( const std::string & node ) { m_rhoNode = node; }
  void add_targetTowerNode( const std::string & node ) { m_targetTowerNodes.push_back(node); }
  void set_subSuffix( const std::string & node ) { m_subSuffix = node; }
  void set_globalVertexType( GlobalVertex::VTXTYPE type ) { m_vertex_type = type; }

 private:

  int CreateNode( PHCompositeNode *topNode );
  int grab_zvrtx( PHCompositeNode *topNode );
  bool m_doFlowMod = false;
  std::string m_rhoNode = "";
  
  std::vector< std::string > m_targetTowerNodes {};
  
  std::string m_subSuffix = "";

  TowerRho::Method m_rho_method = TowerRho::Method::NONE;

  static std::string get_outputTowerNode( const std::string & input_node, const std::string & sub_suffix )
  {
    return  sub_suffix + "_" + input_node; 
  }

  GlobalVertex::VTXTYPE m_vertex_type {GlobalVertex::UNDEFINED};
  float m_vtxz = 0.0;

};

#endif
