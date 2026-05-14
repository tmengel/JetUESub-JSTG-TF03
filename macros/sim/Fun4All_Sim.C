#ifndef _FUN4ALL_SIM_C
#define _FUN4ALL_SIM_C

#include <HIJetReco.C>

#include <Calo_Calib.C>

#include <ffamodules/CDBInterface.h>

#include <fun4all/Fun4AllServer.h>
#include <fun4all/Fun4AllDstInputManager.h>
#include <fun4all/Fun4AllDstOutputManager.h>
#include <fun4all/Fun4AllRunNodeInputManager.h>
#include <fun4all/Fun4AllUtils.h>

#include <phool/recoConsts.h>

#include <mbd/MbdReco.h>
#include <epd/EpdReco.h>
#include <zdcinfo/ZdcReco.h>
#include <globalvertex/GlobalVertexReco.h>
#include <centrality/CentralityReco.h>

#include <calotrigger/MinimumBiasClassifier.h>

#include <eventselection/EventSelector.h>
#include <eventselection/LeadJetCut.h>
#include <eventselection/MinBiasCut.h>
#include <eventselection/TriggerSelect.h>
#include <eventselection/ZVertexCut.h>
#include <eventselection/MissingSebFilter.h>

#include <anatreewriter/SimTree.h>

#include <jetbackground/SubtractTowersRho.h>
#include <jetbackground/DetermineTowerBackgroundv2.h>

R__LOAD_LIBRARY( libfun4all.so )
R__LOAD_LIBRARY( libffamodules.so )
R__LOAD_LIBRARY( libcalotrigger.so )
R__LOAD_LIBRARY( libcentrality.so )
R__LOAD_LIBRARY( libmbd.so )
R__LOAD_LIBRARY( libepd.so )
R__LOAD_LIBRARY( libzdcinfo.so )
R__LOAD_LIBRARY( libglobalvertex.so )
R__LOAD_LIBRARY( libjetbase.so )
R__LOAD_LIBRARY( libeventselection.so )
R__LOAD_LIBRARY( libanatreewriter.so )

namespace HIJETS
{ 
    // additional params 
    float seed_D = 3;
    float seed_pT = 7.0;
    float seed_comp_Cut = 3.0;
    float DR_EXCL = 0.4;
    bool ReweightFlow = true;   
    std::vector< float > Rs = { 0.2, 0.3, 0.4, 0.5 }; 
    TowerJetInput * TowerInput (Jet::SRC src, std::string prefix = HIJETS::tower_prefix ) {   
        auto input = new TowerJetInput(src, prefix);
        input->set_GlobalVertexType(HIJETS::vertex_type);
        return input;
    };
    int psi2_mode = 0;

    std::vector< Jet::SRC > raw_tower_inputs = {
        Jet::CEMC_TOWERINFO_RETOWER,
        Jet::HCALIN_TOWERINFO,
        Jet::HCALOUT_TOWERINFO
    };

    std::vector< Jet::SRC > sub_tower_inputs = {
        Jet::CEMC_TOWERINFO_SUB1,
        Jet::HCALIN_TOWERINFO_SUB1,
        Jet::HCALOUT_TOWERINFO_SUB1
    };

    std::string JetNode( const float R, const bool sub = true)
    {
        if (sub) {
            return Form("%s_TowerInfo_r0%d_Sub1", HIJETS::algo_prefix.c_str(), static_cast<int>(R * 10));
        } else {
            return Form("%s_TowerInfo_r0%d", HIJETS::algo_prefix.c_str(), static_cast<int>(R * 10));
        }
    };
    std::string JetNodeMult( const float R)
    {
        return Form("%s_TowerInfo_r0%d_MultSub", HIJETS::algo_prefix.c_str(), static_cast<int>(R * 10));
    };
    std::string JetNodeArea( const float R)
    {
        return Form("%s_TowerInfo_r0%d_AreaSub", HIJETS::algo_prefix.c_str(), static_cast<int>(R * 10));
    };


    std::string RawSeedNode = "Kt_TowerInfo_HIRecoSeedsRaw_r02";
    std::string Sub1SeedNode = "Kt_TowerInfo_HIRecoSeedsSub_r02";

};

namespace SETTINGS
{
    bool IS_SIM = false;
    bool IS_DATA = false;
    bool FIT_WAVEFORMS = false;
    bool CALIBRATE_CALO = false;
    bool RESCALE_CALOS = false;
};

void GetJets();
void CalcRho();

void Fun4All_Sim ( 
    const int nEvents = 10,
    const int run_number = 31,
    const int segment = 0,
    const std::string & outfile = "test_run31_hijing-00000031-00000.root",
    const std::string & cdbtag = "MDC2",
    const int jet_flag   = 10,
    const int flow_flag  = 1,
    const int psi2_flag  = 3
)
{
    
    std::cout << "Fun4All" << std::endl;

    Enable::VERBOSITY = 0;
    Enable::HIJETS_VERBOSITY = 0;
    SETTINGS::IS_SIM = true;
    SETTINGS::IS_DATA = false;

    SETTINGS::FIT_WAVEFORMS = false; // no waveform fitting in this skimmer
    SETTINGS::CALIBRATE_CALO = true; // always calibrate calo
   
    HIJETS::DR_EXCL = 0.4; 
    HIJETS::tower_prefix = "TOWERINFO_CALIB";
    HIJETS::do_vertex_type = true;
    HIJETS::vertex_type = GlobalVertex::MBD;
    HIJETS::do_flow = flow_flag;
    HIJETS::seed_comp_Cut = 3.0;
    HIJETS::seed_D = 3.0;
    HIJETS::seed_pT = 7.0;
    HIJETS::ReweightFlow = true; // doesn't matter do_flow is false
    HIJETS::Rs = {  0.3 };
    HIJETS::psi2_mode = psi2_flag;

    std::cout << "Fun4All_AuAuJets: Verbosity set to " << Enable::VERBOSITY << std::endl;
    std::cout << "Fun4All_AuAuJets: HIJETS::do_flow = " << HIJETS::do_flow << std::endl;
    std::cout << "IS_SIM = " << SETTINGS::IS_SIM << ", IS_DATA = " << SETTINGS::IS_DATA << std::endl;
    std::cout << "Jet flag = " << jet_flag << std::endl;
    std::cout << "Flow is " << (HIJETS::do_flow ? "ON" : "OFF") << std::endl;
    std::cout << "Psi2 mode is " << HIJETS::psi2_mode << std::endl;


    // Fun4All
    auto se = Fun4AllServer::instance();
    se -> Verbosity( Enable::VERBOSITY  );

    auto rc = recoConsts::instance();
    rc -> set_StringFlag( "CDB_GLOBALTAG", cdbtag );
    rc -> set_uint64Flag( "TIMESTAMP", run_number );
    CDBInterface::instance( ) -> Verbosity( Enable::VERBOSITY );
    

    for ( const auto & DSTTPYE : { "CALO_CLUSTER", "GLOBAL", "MBD_EPD" , "TRUTH_G4HIT" , "TRUTH_JET" } )
    {
        if ( jet_flag <= 0 && std::string(DSTTPYE) == "TRUTH_JET" )
        { 
            continue; 
        } // skip truth jets if not requested

        auto infile = Form( "DST_%s_sHijing_0_20fm-%010d-%06d.root", DSTTPYE, run_number, segment );
        if ( jet_flag > 0 )
        {
            infile = Form( "DST_%s_pythia8_Jet%d_sHijing_0_20fm-%010d-%06d.root", DSTTPYE, jet_flag, run_number, segment );
        }

        std::cout << "\tAdding input file: " << infile << std::endl;

        auto input = new Fun4AllDstInputManager( Form( "DSTINPUT_%s", DSTTPYE ) );
        input -> AddFile( infile );
        input -> Verbosity( Enable::VERBOSITY );
        se -> registerInputManager( input );
    }

    CaloTowerDefs::BuilderType buildertype = CaloTowerDefs::kPRDFTowerv4;
    auto ingeom = new Fun4AllRunNodeInputManager( "DST_GEO" );
    auto geoLocation = CDBInterface::instance() -> getUrl( "calo_geo" );
    ingeom -> AddFile( geoLocation );
    se -> registerInputManager( ingeom );

    if ( SETTINGS::CALIBRATE_CALO ) { Process_Calo_Calib(); }


    if ( SETTINGS::IS_DATA )  
    {
        auto mbdreco = new MbdReco();
        se -> registerSubsystem( mbdreco );
      
        auto epdreco = new EpdReco();
        epdreco -> Verbosity( Enable::VERBOSITY );
        se -> registerSubsystem( epdreco );
      
        auto gvertex = new GlobalVertexReco();
        se -> registerSubsystem( gvertex );
      
        auto zdcreco = new ZdcReco();
        zdcreco -> set_zdc1_cut(0.0);
        zdcreco -> set_zdc2_cut(0.0);
        se -> registerSubsystem( zdcreco );
    }

    auto rcemc = new RetowerCEMC( ); 
    rcemc -> set_towerinfo( true );
    rcemc -> set_frac_cut( 0.5 ); 
    rcemc -> set_towerNodePrefix( HIJETS::tower_prefix );
    rcemc -> Verbosity( Enable::VERBOSITY );
    se -> registerSubsystem( rcemc );

    

    auto ep = new EventPlaneReco( );
    ep -> Verbosity( Enable::VERBOSITY );
    if ( SETTINGS::IS_SIM ) 
    {
        ep -> set_inputNode( "TOWERINFO_CALIB_EPD" );
    }
    se -> registerSubsystem( ep );    

    auto mb = new MinimumBiasClassifier( );
    mb -> Verbosity( Enable::VERBOSITY );
    if ( SETTINGS::IS_SIM ) 
    {
        mb -> setIsSim( true );
        mb -> setOverwriteScale( "/sphenix/user/dlis/Projects/centrality/cdb/calibrations/scales/cdb_centrality_scale_1.root" );
        mb -> setOverwriteVtx( "/sphenix/user/dlis/Projects/centrality/cdb/calibrations/vertexscales/cdb_centrality_vertex_scale_1.root" );
    }
    se -> registerSubsystem( mb );

    auto cr = new CentralityReco( );
    cr -> Verbosity( Enable::VERBOSITY );
    if ( SETTINGS::IS_SIM ) 
    {
        cr -> setOverwriteScale( "/sphenix/user/dlis/Projects/centrality/cdb/calibrations/scales/cdb_centrality_scale_1.root" );
        cr -> setOverwriteVtx( "/sphenix/user/dlis/Projects/centrality/cdb/calibrations/vertexscales/cdb_centrality_vertex_scale_1.root" );
        cr -> setOverwriteDivs( "/sphenix/user/dlis/Projects/centrality/cdb/calibrations/divs/cdb_centrality_1.root" );
    }
    se -> registerSubsystem( cr );

    auto es = new EventSelector( );
    es -> Verbosity( Enable::VERBOSITY );

    auto mbc = new MinBiasCut( );
    mbc -> SetNodeName( "MinimumBiasInfo" );
    es -> AddCut( mbc );

    auto zvc = new ZVertexCut( 60, -60 );
    zvc -> SetNodeName( "GlobalVertexMap" );
    es -> AddCut( zvc );

    es -> PrintCuts( );
    se -> registerSubsystem( es );

    GetJets();


    auto anaout = new SimTree( outfile );
    anaout -> add_zvrtx_node ( "GlobalVertexMap" );
    anaout -> add_cent_node ( "CentralityInfo" );
    anaout -> add_event_header ( "EventHeader" );
    anaout -> add_sub1jet_node ( HIJETS::JetNode( 0.3, true ) );
    anaout -> add_multjet_node ( HIJETS::JetNodeMult( 0.3 ) );
    // anaout -> add_areajet_node ( HIJETS::JetNodeArea( 0.3 ) );
    anaout -> add_rawjet_node ( HIJETS::JetNode( 0.3, false ) );
    anaout -> add_truthnode ( "G4TruthInfo" );
    anaout -> add_ep_info ( "EventplaneinfoMap" );
    anaout -> do_towerbkgd( true );
    anaout -> do_rho( true );
    if ( jet_flag > 0 )
    {
       anaout -> add_truthjet_node ( Form( "AntiKt_Truth_r03" ) );
    }
    anaout -> Verbosity( Enable::VERBOSITY  );
    se -> registerSubsystem( anaout );


    se -> run( nEvents );
    se -> End( );   
    se -> PrintTimer( );


    CDBInterface::instance() -> Print();

    delete se;

    std::cout << "Done4All" << std::endl;

    gSystem -> Exit( 0 );

}

void GetJets()
{


    int verbosity = std::max( Enable::VERBOSITY, Enable::HIJETS_VERBOSITY );
   
    if ( verbosity > 0 ) {
        std::cout << "Starting GetJets" << std::endl;
    }

    auto se = Fun4AllServer::instance();
    
    auto towerjetreco = new JetReco( );
    for ( const auto & src : HIJETS::raw_tower_inputs ) { towerjetreco -> add_input( HIJETS::TowerInput( src ) ); }
    FastJetOptions fj_opts( {Jet::KT, 0.4, 0} );
    towerjetreco -> add_algo( new FastJetAlgoSub( fj_opts ) , HIJETS::RawSeedNode );
    towerjetreco -> set_algo_node( HIJETS::jet_node );
    towerjetreco -> set_input_node( "TOWER" );
    towerjetreco -> Verbosity( verbosity );
    se -> registerSubsystem( towerjetreco );

    auto dtb = new DetermineTowerBackgroundv2( );
    dtb -> SetBackgroundOutputName( "TowerInfoBackground_Sub1" );
    dtb -> SetFlowMode( HIJETS::do_flow );
    dtb -> SetPsi2Mode( HIJETS::psi2_mode );
    dtb -> SetIHCAL_TowerInfoNode( "TOWERINFO_CALIB_HCALIN" );
    dtb -> SetOHCAL_TowerInfoNode( "TOWERINFO_CALIB_HCALOUT" );
    dtb -> SetCEMC_RetowerInfoNode( "TOWERINFO_CALIB_CEMC_RETOWER" );
    dtb -> SetSeedJetName( HIJETS::RawSeedNode );
    dtb -> SetSeedType( 0 );
    dtb -> SetSeedJetPt( 5.0 );
    dtb -> SetNOmitSeeds( 2 );
    dtb -> Verbosity( 0 ); 
    se  -> registerSubsystem( dtb );

    auto casj = new CopyAndSubtractJets( );
    casj -> SetFlowModulation( HIJETS::do_flow );
    casj -> set_rawseed_node( HIJETS::RawSeedNode );
    casj -> set_subseed_node( HIJETS::Sub1SeedNode );
    casj -> set_background_node( "TowerInfoBackground_Sub1" );
    casj -> set_jet_node( HIJETS::jet_node );
    casj -> set_input_node( "TOWER" );
    casj -> Verbosity( verbosity ); 
    casj -> set_towerinfo( true );
    casj -> set_towerNodePrefix( HIJETS::tower_prefix );
    se   -> registerSubsystem( casj );

    auto dtb3 = new DetermineTowerBackgroundv2( "DetermineTowerBackground_Sub2" );
    dtb3 -> SetBackgroundOutputName( "TowerInfoBackground_Sub2" );
    dtb3 -> SetFlowMode( HIJETS::do_flow );
    dtb3 -> SetPsi2Mode( HIJETS::psi2_mode );
    dtb3 -> SetIHCAL_TowerInfoNode( "TOWERINFO_CALIB_HCALIN" );
    dtb3 -> SetOHCAL_TowerInfoNode( "TOWERINFO_CALIB_HCALOUT" );
    dtb3 -> SetCEMC_RetowerInfoNode( "TOWERINFO_CALIB_CEMC_RETOWER" );
    dtb3 -> SetSeedJetName( HIJETS::Sub1SeedNode );
    dtb3 -> SetSeedType( 1 );
    dtb3 -> SetSeedJetPt( 7.0 );
    dtb3 -> SetNOmitSeeds( 2 );
    dtb3 -> Verbosity( 0 );
    se  -> registerSubsystem( dtb3 );
    
    // auto dtb2 = new DetermineTowerBackground( );
    // dtb2 -> SetBackgroundOutputName( "TowerInfoBackground_Sub2" );
    // dtb2 -> SetFlow( HIJETS::do_flow );
    // dtb2 -> SetSeedExclusionDR( HIJETS::DR_EXCL); 
    // // dtb2 -> SetOverwriteCaloV2( overwrite_cdb );
    // dtb2 -> SetSeedType( 1 );
    // dtb2 -> SetSeedJetPt( HIJETS::seed_pT );
    // dtb2 -> Verbosity( verbosity );
    // dtb2 -> UseReweighting( HIJETS::ReweightFlow );
    // dtb2 -> set_towerNodePrefix( HIJETS::tower_prefix );
    // se   -> registerSubsystem( dtb2 );
    
    auto st = new SubtractTowers( );
    st -> SetFlowModulation( HIJETS::do_flow );
    st -> Verbosity( verbosity );
    st -> set_towerinfo( true );
    st -> set_towerNodePrefix( HIJETS::tower_prefix );
    se -> registerSubsystem( st );

    towerjetreco = new JetReco( );
    for ( const auto & src : HIJETS::sub_tower_inputs ) { towerjetreco -> add_input( HIJETS::TowerInput( src ) ); }
    for ( const auto &R : HIJETS::Rs ) { towerjetreco -> add_algo( HIJETS::GetFJAlgo(R), HIJETS::JetNode(R, true) ); }
    towerjetreco -> set_algo_node( HIJETS::jet_node );
    towerjetreco -> set_input_node( "TOWER" );
    towerjetreco -> Verbosity( verbosity );
    se -> registerSubsystem( towerjetreco );

    towerjetreco = new JetReco( );
    for ( const auto & src : HIJETS::raw_tower_inputs ) { towerjetreco -> add_input( HIJETS::TowerInput( src ) ); }
    for ( const auto &R : HIJETS::Rs ) { towerjetreco -> add_algo( HIJETS::GetFJAlgo(R), HIJETS::JetNode(R, false) ); }
    towerjetreco -> set_algo_node( HIJETS::jet_node );
    towerjetreco -> set_input_node( "TOWER" );
    towerjetreco -> Verbosity( verbosity );
    se -> registerSubsystem( towerjetreco );

   
    auto towRhoCalc = new DetermineTowerRho();
    towRhoCalc -> add_method( TowerRho::Method::MULT, "TowerRho_MULT_CEMC" );
    towRhoCalc -> add_tower_input( HIJETS::TowerInput( Jet::CEMC_TOWERINFO ) );
    se -> registerSubsystem( towRhoCalc );

    auto subrho = new SubtractTowersRho();    
    subrho -> set_rhoNode("TowerRho_MULT_CEMC");
    subrho -> add_targetTowerNode( "TOWERINFO_CALIB_CEMC" );
    subrho -> set_subSuffix( "MULTSUB" );
    subrho -> set_globalVertexType( HIJETS::vertex_type );
    subrho -> Verbosity( verbosity );
    se -> registerSubsystem( subrho );

    towRhoCalc = new DetermineTowerRho();
    towRhoCalc -> add_method( TowerRho::Method::MULT, "TowerRho_MULT_HCALIN" );
    towRhoCalc -> add_tower_input( HIJETS::TowerInput( Jet::HCALIN_TOWERINFO ) );
    se -> registerSubsystem( towRhoCalc );  

    subrho = new SubtractTowersRho();
    subrho -> set_rhoNode("TowerRho_MULT_HCALIN");
    subrho -> add_targetTowerNode( "TOWERINFO_CALIB_HCALIN" );
    subrho -> set_subSuffix( "MULTSUB" );
    subrho -> set_globalVertexType( HIJETS::vertex_type );
    subrho -> Verbosity( verbosity );
    se -> registerSubsystem( subrho );

    towRhoCalc = new DetermineTowerRho();
    towRhoCalc -> add_method( TowerRho::Method::MULT, "TowerRho_MULT_HCALOUT" );
    towRhoCalc -> add_tower_input( HIJETS::TowerInput( Jet::HCALOUT_TOWERINFO ) );
    se -> registerSubsystem( towRhoCalc );

    subrho = new SubtractTowersRho();
    subrho -> set_rhoNode("TowerRho_MULT_HCALOUT");
    subrho -> add_targetTowerNode( "TOWERINFO_CALIB_HCALOUT" );
    subrho -> set_subSuffix( "MULTSUB" );
    subrho -> set_globalVertexType( HIJETS::vertex_type );
    subrho -> Verbosity( verbosity );
    se -> registerSubsystem( subrho );

    towRhoCalc = new DetermineTowerRho();
    towRhoCalc -> add_method( TowerRho::Method::MULT, "TowerRho_MULT" );
    towRhoCalc -> add_tower_input( HIJETS::TowerInput( Jet::CEMC_TOWERINFO ) );
    towRhoCalc -> add_tower_input( HIJETS::TowerInput( Jet::HCALIN_TOWERINFO ) );
    towRhoCalc -> add_tower_input( HIJETS::TowerInput( Jet::HCALOUT_TOWERINFO ) );
    se -> registerSubsystem( towRhoCalc );

    towRhoCalc = new DetermineTowerRho();
    towRhoCalc -> add_method( TowerRho::Method::AREA, "TowerRho_AREA_CEMC" );
    towRhoCalc -> add_tower_input( HIJETS::TowerInput( Jet::CEMC_TOWERINFO_RETOWER ) );
    se -> registerSubsystem( towRhoCalc );

    subrho = new SubtractTowersRho();
    subrho -> set_rhoNode("TowerRho_AREA_CEMC");
    subrho -> add_targetTowerNode( "TOWERINFO_CALIB_CEMC_RETOWER" );
    subrho -> set_subSuffix( "AREASUB" );
    subrho -> set_globalVertexType( HIJETS::vertex_type );
    subrho -> Verbosity( verbosity  );
    se -> registerSubsystem( subrho );

    towRhoCalc = new DetermineTowerRho();
    towRhoCalc -> add_method( TowerRho::Method::AREA, "TowerRho_AREA_HCALIN" );
    towRhoCalc -> add_tower_input( HIJETS::TowerInput( Jet::HCALIN_TOWERINFO ) );
    se -> registerSubsystem( towRhoCalc );

    subrho = new SubtractTowersRho();
    subrho -> set_rhoNode("TowerRho_AREA_HCALIN");
    subrho -> add_targetTowerNode( "TOWERINFO_CALIB_HCALIN" );
    subrho -> set_subSuffix( "AREASUB" );
    subrho -> set_globalVertexType( HIJETS::vertex_type );
    subrho -> Verbosity( verbosity );
    se -> registerSubsystem( subrho );

    towRhoCalc = new DetermineTowerRho();
    towRhoCalc -> add_method( TowerRho::Method::AREA, "TowerRho_AREA_HCALOUT" );
    towRhoCalc -> add_tower_input( HIJETS::TowerInput( Jet::HCALOUT_TOWERINFO ) );
    se -> registerSubsystem( towRhoCalc );

    subrho = new SubtractTowersRho();
    subrho -> set_rhoNode("TowerRho_AREA_HCALOUT");
    subrho -> add_targetTowerNode( "TOWERINFO_CALIB_HCALOUT" );
    subrho -> set_subSuffix( "AREASUB" );
    subrho -> set_globalVertexType( HIJETS::vertex_type );
    subrho -> Verbosity( verbosity );
    se -> registerSubsystem( subrho );  

    towRhoCalc = new DetermineTowerRho();  
    towRhoCalc -> add_method( TowerRho::Method::AREA, "TowerRho_AREA" );
    towRhoCalc -> add_tower_input( HIJETS::TowerInput( Jet::CEMC_TOWERINFO_RETOWER ) );
    towRhoCalc -> add_tower_input( HIJETS::TowerInput( Jet::HCALIN_TOWERINFO ) );
    towRhoCalc -> add_tower_input( HIJETS::TowerInput( Jet::HCALOUT_TOWERINFO ) );
    se -> registerSubsystem( towRhoCalc );

    // reconstruct jets with mult-subtracted towers
    towerjetreco = new JetReco( );
    for ( const auto & src : { Jet::CEMC_TOWERINFO, Jet::HCALIN_TOWERINFO, Jet::HCALOUT_TOWERINFO } ) { towerjetreco -> add_input( HIJETS::TowerInput( src , "MULTSUB_" + HIJETS::tower_prefix ) ); }
    for ( const auto &R : HIJETS::Rs ) { towerjetreco -> add_algo( HIJETS::GetFJAlgo(R), HIJETS::JetNodeMult(R) ); }
    towerjetreco -> set_algo_node( HIJETS::jet_node );
    towerjetreco -> set_input_node( "TOWER" );
    towerjetreco -> Verbosity( verbosity );
    se -> registerSubsystem( towerjetreco );

    // area
    towerjetreco = new JetReco( );
    for ( const auto & src : { Jet::CEMC_TOWERINFO_RETOWER, Jet::HCALIN_TOWERINFO, Jet::HCALOUT_TOWERINFO } ) { towerjetreco -> add_input( HIJETS::TowerInput( src , "AREASUB_" + HIJETS::tower_prefix ) ); }
    for ( const auto &R : HIJETS::Rs ) { towerjetreco -> add_algo( HIJETS::GetFJAlgo(R), HIJETS::JetNodeArea(R) ); }
    towerjetreco -> set_algo_node( HIJETS::jet_node );
    towerjetreco -> set_input_node( "TOWER" );
    towerjetreco -> Verbosity( verbosity );
    se -> registerSubsystem( towerjetreco );

    if ( verbosity > 0 ) {
        std::cout << "Done GetJets." << std::endl;
    }
    return;

}

void CalcRho()
{
    if ( Enable::VERBOSITY > 0 ) 
    {
        std::cout << "Starting CalcRho" << std::endl;
    }
    auto se = Fun4AllServer::instance();

    auto towRhoCalc = new DetermineTowerRho();
    towRhoCalc -> add_method( TowerRho::Method::MULT, "TowerRho_MULT_CEMC" );
    towRhoCalc -> add_tower_input( HIJETS::TowerInput( Jet::CEMC_TOWERINFO_RETOWER ) );
    se -> registerSubsystem( towRhoCalc );
    
    towRhoCalc = new DetermineTowerRho();
    towRhoCalc -> add_method( TowerRho::Method::MULT, "TowerRho_MULT_HCALIN" );
    towRhoCalc -> add_tower_input( HIJETS::TowerInput( Jet::HCALIN_TOWERINFO ) );
    se -> registerSubsystem( towRhoCalc );  

    towRhoCalc = new DetermineTowerRho();
    towRhoCalc -> add_method( TowerRho::Method::MULT, "TowerRho_MULT_HCALOUT" );
    towRhoCalc -> add_tower_input( HIJETS::TowerInput( Jet::HCALOUT_TOWERINFO ) );
    se -> registerSubsystem( towRhoCalc );

    towRhoCalc = new DetermineTowerRho();
    towRhoCalc -> add_method( TowerRho::Method::MULT, "TowerRho_MULT" );
    towRhoCalc -> add_tower_input( HIJETS::TowerInput( Jet::CEMC_TOWERINFO_RETOWER ) );
    towRhoCalc -> add_tower_input( HIJETS::TowerInput( Jet::HCALIN_TOWERINFO ) );
    towRhoCalc -> add_tower_input( HIJETS::TowerInput( Jet::HCALOUT_TOWERINFO ) );
    se -> registerSubsystem( towRhoCalc );

    towRhoCalc = new DetermineTowerRho();
    towRhoCalc -> add_method( TowerRho::Method::AREA, "TowerRho_AREA_CEMC" );
    towRhoCalc -> add_tower_input( HIJETS::TowerInput( Jet::CEMC_TOWERINFO_RETOWER ) );
    se -> registerSubsystem( towRhoCalc );

    towRhoCalc = new DetermineTowerRho();
    towRhoCalc -> add_method( TowerRho::Method::AREA, "TowerRho_AREA_HCALIN" );
    towRhoCalc -> add_tower_input( HIJETS::TowerInput( Jet::HCALIN_TOWERINFO ) );
    se -> registerSubsystem( towRhoCalc );

    towRhoCalc = new DetermineTowerRho();
    towRhoCalc -> add_method( TowerRho::Method::AREA, "TowerRho_AREA_HCALOUT" );
    towRhoCalc -> add_tower_input( HIJETS::TowerInput( Jet::HCALOUT_TOWERINFO ) );
    se -> registerSubsystem( towRhoCalc );
    
    towRhoCalc = new DetermineTowerRho();  
    towRhoCalc -> add_method( TowerRho::Method::AREA, "TowerRho_AREA" );
    towRhoCalc -> add_tower_input( HIJETS::TowerInput( Jet::CEMC_TOWERINFO_RETOWER ) );
    towRhoCalc -> add_tower_input( HIJETS::TowerInput( Jet::HCALIN_TOWERINFO ) );
    towRhoCalc -> add_tower_input( HIJETS::TowerInput( Jet::HCALOUT_TOWERINFO ) );
    se -> registerSubsystem( towRhoCalc );

    if ( Enable::VERBOSITY > 0 ) {
        std::cout << "Done CalcRho." << std::endl;
    }
    return;

}

#endif // FUN4ALL_C
