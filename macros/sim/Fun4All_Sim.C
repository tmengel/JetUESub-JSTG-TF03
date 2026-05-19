#ifndef _FUN4ALL_SIM_C_
#define _FUN4ALL_SIM_C_

#include "HIJetReco.C"

#include <Calo_Calib.C>

#include <Trkr_Reco.C>

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

R__LOAD_LIBRARY( libfun4all.so )
R__LOAD_LIBRARY( libffamodules.so )
R__LOAD_LIBRARY( libcalotrigger.so )
R__LOAD_LIBRARY( libcentrality.so )
R__LOAD_LIBRARY( libmbd.so )
R__LOAD_LIBRARY( libepd.so )
R__LOAD_LIBRARY( libzdcinfo.so )
R__LOAD_LIBRARY( libglobalvertex.so )
R__LOAD_LIBRARY( libjetbase.so )


void Fun4All_Sim ( 
    const int nEvents = 10,
    const int run_number = 31,
    const int segment = 0,
    const std::string & outfile = "testout.root"
)
{

    const std::string cdbtag = "MDC2";
    const int jet_flag       = 10;

    std::cout << "Fun4All" << std::endl;

    Enable::VERBOSITY = 0;
   
    
    // Fun4All
    auto se = Fun4AllServer::instance();
    se -> Verbosity( Enable::VERBOSITY  );

    auto rc = recoConsts::instance();
    rc -> set_StringFlag( "CDB_GLOBALTAG", cdbtag );
    rc -> set_uint64Flag( "TIMESTAMP", run_number );
    CDBInterface::instance( ) -> Verbosity( Enable::VERBOSITY );

    for ( const auto & DSTTPYE : { "CALO_CLUSTER",  "MBD_EPD" , "GLOBAL", "TRUTH_JET" } )
    {
        // skip truth jets if not requested
        if ( jet_flag <= 0 && std::string(DSTTPYE) == "TRUTH_JET" )
        { 
            continue; 
        } 
        auto infile = Form( "DST_%s_sHijing_0_20fm-%010d-%06d.root", DSTTPYE, run_number, segment );
        if ( jet_flag > 0 )
        {
            // embedded
            infile = Form( "DST_%s_pythia8_Jet%d_sHijing_0_20fm-%010d-%06d.root", DSTTPYE, jet_flag, run_number, segment );
        }
        std::cout << "\tAdding input file: " << infile << std::endl;

        auto input = new Fun4AllDstInputManager( Form( "DSTINPUT_%s", DSTTPYE ) );
        input -> AddFile( infile );
        input -> Verbosity( Enable::VERBOSITY );
        se -> registerInputManager( input );
    }

    // calo geom
    CaloTowerDefs::BuilderType buildertype = CaloTowerDefs::kPRDFTowerv4;
    
    auto ingeom = new Fun4AllRunNodeInputManager( "DST_GEO" );
    auto geoLocation = CDBInterface::instance() -> getUrl( "calo_geo" );
    ingeom -> AddFile( geoLocation );
    se -> registerInputManager( ingeom );

    // calibrate
    Process_Calo_Calib(); 

    auto ep = new EventPlaneReco( );
    ep -> set_inputNode( "TOWERINFO_CALIB_EPD" );
    ep -> Verbosity( Enable::VERBOSITY );
    se -> registerSubsystem( ep );    

    auto mb = new MinimumBiasClassifier( );
    mb -> setIsSim( true );
    mb -> setOverwriteScale( "/sphenix/user/dlis/Projects/centrality/cdb/calibrations/scales/cdb_centrality_scale_1.root" );
    mb -> setOverwriteVtx( "/sphenix/user/dlis/Projects/centrality/cdb/calibrations/vertexscales/cdb_centrality_vertex_scale_1.root" );
    mb -> Verbosity( Enable::VERBOSITY );
    se -> registerSubsystem( mb );

    auto cr = new CentralityReco( );
    cr -> setOverwriteScale( "/sphenix/user/dlis/Projects/centrality/cdb/calibrations/scales/cdb_centrality_scale_1.root" );
    cr -> setOverwriteVtx( "/sphenix/user/dlis/Projects/centrality/cdb/calibrations/vertexscales/cdb_centrality_vertex_scale_1.root" );
    cr -> setOverwriteDivs( "/sphenix/user/dlis/Projects/centrality/cdb/calibrations/divs/cdb_centrality_1.root" );
    cr -> Verbosity( Enable::VERBOSITY );
    se -> registerSubsystem( cr );


    // this flag alone will allow the configuration of DTB
    // changing anything else if this is false will do nothing!
    // Enable::HIJETS_ENABLE_TEST = true; 

    // 0 - default
    
    // 1 - disable exclude full eta strip in flow calc,  renormalize eta strip in flow calc
    // 2 - R = 0.4, kT seeds
    // 3 - limit to 2 seeds     
    // 4 - positive E tower cut, require positive UE

    // 5 - dab on them 
    int HIJET_TEST_NUMBER = 0;

    // default settings 
    Enable::HIJETS_VERBOSITY = 0;
   
    HIJETS::do_flow = 3; // event by event

    HIJETS::seed_algo = Jet::ANTIKT; 
    HIJETS::seed_R = 0.2;
    HIJETS::n_omit_seeds = -1;
    HIJETS::DR = 0.4; // -4 will go to tower excludsion
    HIJETS::exclude_full_eta_flow_strips = true;
    HIJETS::min_tower_energy = -999; // < 0 means disabled
    HIJETS::renormalize_eta_strip = false; // if true, divide tower energies by average energy in strip to account for eta dependence of background
    HIJETS::do_double_check_on_UE = false; // < ensures nonnegative UE 
    HIJETS::do_reweight = true; // should just be true without option to change

    if ( HIJET_TEST_NUMBER == 1 )
    {
        // turn off full eta exclusion
        HIJETS::exclude_full_eta_flow_strips = false;
        HIJETS::renormalize_eta_strip = true;
    }
    else if ( HIJET_TEST_NUMBER == 2 )
    {
        // do kT seeds
        HIJETS::seed_algo = Jet::KT;
        HIJETS::seed_R = 0.4;
        HIJETS::DR = -1.0; // only exclude towers in jet, not full DR exclusion
    }
    else if ( HIJET_TEST_NUMBER == 3 )
    {
        // limit to 2 seeds (normal seed settings)
        HIJETS::n_omit_seeds = 2;
    }
    else if ( HIJET_TEST_NUMBER == 4 )
    {
        // enforce positive energy towers and positive UE
        HIJETS::min_tower_energy = 0.0;
        HIJETS::do_double_check_on_UE = true;
    }
    else if ( HIJET_TEST_NUMBER == 5 )
    {
        // dab on them
        HIJETS::seed_algo = Jet::KT;
        HIJETS::seed_R = 0.4;
        HIJETS::DR = -1.0; // only exclude towers in jet, not full DR exclusion
        HIJETS::n_omit_seeds = 2;
        HIJETS::exclude_full_eta_flow_strips = false;
        HIJETS::renormalize_eta_strip = true;
        HIJETS::min_tower_energy = 0.0;
        HIJETS::do_double_check_on_UE = true;
    }

   

    HIJetReco();

    // auto anaout = new SimTree( outfile );
    // anaout -> add_zvrtx_node ( "GlobalVertexMap" );
    // anaout -> add_cent_node ( "CentralityInfo" );
    // anaout -> add_event_header ( "EventHeader" );
    // anaout -> add_sub1jet_node ( HIJETS::JetNode( 0.3, true ) );
    // anaout -> add_truthnode ( "G4TruthInfo" );
    // anaout -> add_ep_info ( "EventplaneinfoMap" );
    // anaout -> do_towerbkgd( true );
    // anaout -> do_rho( true );
    // if ( jet_flag > 0 )
    // {
    //    anaout -> add_truthjet_node ( Form( "AntiKt_Truth_r03" ) );
    // }
    // anaout -> Verbosity( Enable::VERBOSITY  );
    // se -> registerSubsystem( anaout );

    se -> run( nEvents );
    se -> End( );   
    se -> PrintTimer( );


    CDBInterface::instance() -> Print();

    delete se;

    std::cout << "Done4All" << std::endl;

    gSystem -> Exit( 0 );

}


#endif // _FUN4ALL_SIM_C_
