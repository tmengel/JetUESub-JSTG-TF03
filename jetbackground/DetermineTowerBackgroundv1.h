#ifndef JETBACKGROUND_DetermineTowerBackgroundv1_H
#define JETBACKGROUND_DetermineTowerBackgroundv1_H

#include <fun4all/SubsysReco.h>

#include <jetbase/Jet.h>

#include <calobase/RawTowerDefs.h>

#include <string>
#include <vector>
#include <array>

#include <globalvertex/GlobalVertex.h>

// forward declarations
class PHCompositeNode;
class TowerBackground;
class TowerInfoContainer;
class RawTowerGeomContainer;

class GlobalVertex;

class DetermineTowerBackgroundv1 : public SubsysReco
{
 public:

  enum FlowMode { NoFlow = 0, EBE = 1, AVG = 2};
  enum Psi2Mode { NoPsi2 = 0, Calo = 1, Truth = 2, sEPD = 3 };

  DetermineTowerBackgroundv1(const std::string &name = "DetermineTowerBackgroundv1");
  ~DetermineTowerBackgroundv1() override {}

  int InitRun(PHCompositeNode *topNode) override;
  int process_event(PHCompositeNode *topNode) override;

  void SetVertexType( GlobalVertex::VTXTYPE vertex_type) { m_vertex_type = {vertex_type}; }

  void SetBackgroundOutputName(const std::string &name) { m_background_node = name; }
  
  void SetFlowMode(int mode) { m_flow_mode = mode; }
  void SetPsi2Mode(int mode) { m_psi2_mode = mode; }

  void UseReweighting(bool do_reweight ) {  m_do_reweight = do_reweight; }
  void ExcludeFullEtaFlowStrips(bool exclude) { m_exclude_full_eta_flow_strips = exclude; }
  void SetMinTowerEnergy(float energy) { m_min_tower_energy = energy; }
  void SetRenormalizeEtaStrip(bool renormalize) { m_renormalize_eta_strip = renormalize; }
  void SetDoDoubleCheckOnUE(bool do_double_check_on_ue) { m_do_double_check_on_UE = do_double_check_on_ue; }

  void SetSeedType(int seed_type) { m_seed_type = seed_type; }
  void SetSeedJetName(std::string &name) { m_seed_jet_name = name; }
  
  void SetNOmitSeeds(int n) { m_n_omit_seeds = n; } 

  void SetSeedJetD(float D) { m_seed_jet_D = D; };
  void SetSeedJetPt(float pt) { m_seed_jet_pt = pt; };
  void SetSeedMaxConst(float max_const) { m_seed_max_const = max_const; };
  void SetSeedExclusionDR(float dr) { m_seed_exclusion_DR = dr; };

  void SetOverwriteCaloV2(std::string &url) { m_overwrite_average_calo_v2_path = url; }
  void SetCalibName(const std::string &name) { m_calib_name = name; }
  void SetIHCAL_GeomNode(const std::string &name) { m_ihcal_geom_node = name; }
  void SetOHCAL_GeomNode(const std::string &name) { m_ohcal_geom_node = name; }
  void SetCEMC_GeomNode(const std::string &name) { m_cemc_geom_node = name; } 
  void SetIHCAL_TowerInfoNode(const std::string &name) { m_ihcal_towerinfo_node = name; }
  void SetOHCAL_TowerInfoNode(const std::string &name) { m_ohcal_towerinfo_node = name; }
  void SetCEMC_TowerInfoNode(const std::string &name) { m_cemc_towerinfo_node = name; }
  void SetCEMC_RetowerInfoNode(const std::string &name) { m_cemc_retowerinfo_node = name; }

 private:

  int CreateNode(PHCompositeNode *topNode);
  void FillNode(PHCompositeNode *topNode, const std::string &name);
  TowerInfoContainer * LoadTowerInfoContainer(PHCompositeNode *topNode, const std::string &name);
  RawTowerGeomContainer * LoadTowerGeomContainer(PHCompositeNode *topNode, const std::string &name);
  int get_zvrtx(PHCompositeNode *topNode);
  int init_event(PHCompositeNode *topNode);
  int get_psi2(PHCompositeNode *topNode);
  int get_v2(PHCompositeNode *topNode);
  int LoadCalibrations();
  int fill_energy_vectors(PHCompositeNode *topNode, Jet::SRC src);

  std::string m_background_node       = "TestTowerBackground";
  std::string m_seed_jet_name {""};
  std::string m_overwrite_average_calo_v2_path {""};

  int m_flow_mode = FlowMode::NoFlow;
  int m_psi2_mode = Psi2Mode::sEPD;

  int m_seed_type     = 0; // 0 = D, 1 = pT
  int m_n_omit_seeds  = -1; // if -1 then use typical criteria
  
  float m_seed_jet_D{3.0};
  float m_seed_max_const{3.0};
  float m_seed_jet_pt = 5.0;

  
  std::vector<float> m_cent_avg_v2 {};

  bool m_is_flow_failure = false;
  float m_v2    = 0;
  float m_psi2  = 0;
  int m_nstrips = 0;
  int m_ntowers = 0;

  float m_ihcal_r = 0.0;
  float m_ohcal_r = 0.0;
  float m_emcal_r = 0.0;

  int m_num_eta_ihcal = 0;
  int m_num_phi_ihcal = 0;

  std::vector<float> m_eta_bin_edges {};
  std::vector<float> m_z0_bin_edges {};

  std::vector<std::vector<float>> m_emcal_energy {};
  std::vector<std::vector<float>> m_ihcal_energy {};
  std::vector<std::vector<float>> m_ohcal_energy {};
  
  std::vector<std::vector<bool>> m_emcal_mask {};
  std::vector<std::vector<bool>> m_ihcal_mask {};
  std::vector<std::vector<bool>> m_ohcal_mask {};

  std::vector<std::vector<bool>> m_emcal_flow_mask {};
  std::vector<std::vector<bool>> m_ihcal_flow_mask {};
  std::vector<std::vector<bool>> m_ohcal_flow_mask {};

  std::vector<std::vector<float>> m_ue_density {};


  float m_seed_exclusion_DR = -1.0; // -1 means exclude towers in jet not dR exclusion
  bool  m_exclude_full_eta_flow_strips = true; // if true, require full phi coverage in strip to use for flow extraction, otherwise just exclude towers with no hits
  float m_min_tower_energy   = -999.0;
  bool  m_renormalize_eta_strip = true; // divide by avg E in strip
  bool  m_do_reweight = true;
  bool  m_do_double_check_on_UE = false; // if true, after flow modulation, check if any towers that were used in flow extraction now have energy density above threshold, if so set flow failure flag and set v2 and psi2 to 0

  std::vector<Jet::SRC> m_jetsrcs { Jet::SRC::HCALIN_TOWERINFO, Jet::SRC::HCALOUT_TOWERINFO , Jet::SRC::CEMC_TOWERINFO_RETOWER };

  bool first = true;

  float m_vtxz = 0.0;
  std::vector<GlobalVertex::VTXTYPE> m_vertex_type = {GlobalVertex::MBD};

  RawTowerDefs::CalorimeterId m_caloid = RawTowerDefs::CalorimeterId::NONE;
  TowerInfoContainer    * m_towerinfos = nullptr;
  RawTowerGeomContainer * m_towergeom = nullptr;
  RawTowerGeomContainer * m_ihcal_geom = nullptr;    

  Jet::PROPERTY m_idx_seedD    = Jet::PROPERTY::no_property;
  Jet::PROPERTY m_idx_seed_itr = Jet::PROPERTY::no_property;

  std::string m_calib_name            = "JET_AVERAGE_CALO_V2_SEPD_PSI2";
  std::string m_ihcal_geom_node       = "TOWERGEOM_HCALIN";
  std::string m_ohcal_geom_node       = "TOWERGEOM_HCALOUT";
  std::string m_cemc_geom_node        = "TOWERGEOM_CEMC";
  std::string m_ihcal_towerinfo_node  = "TOWERINFO_CALIB_HCALIN";
  std::string m_ohcal_towerinfo_node  = "TOWERINFO_CALIB_HCALOUT";
  std::string m_cemc_towerinfo_node   = "TOWERINFO_CALIB_CEMC";
  std::string m_cemc_retowerinfo_node = "TOWERINFO_CALIB_CEMC_RETOWER";

};

#endif
