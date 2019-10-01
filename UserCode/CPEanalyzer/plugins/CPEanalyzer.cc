// -*- C++ -*-
//
// Package:    UserCode/CPEanalyzer
// Class:      CPEanalyzer
//
/**\class CPEanalyzer CPEanalyzer.cc UserCode/CPEanalyzer/plugins/CPEanalyzer.cc

 Description: CPE analysis (resolution param, etc.)

 Implementation:
     Used for DPG studies
*/
//
// Original Author:  Christophe Delaere
//         Created:  Thu, 12 Sep 2019 13:51:00 GMT
//
//

// system include files
#include <memory>
#include <iostream>

// user include files
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/one/EDAnalyzer.h"

#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"

#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/InputTag.h"
#include "DataFormats/TrackReco/interface/Track.h"
#include "DataFormats/TrackReco/interface/TrackFwd.h"
#include "TrackingTools/PatternTools/interface/Trajectory.h"
#include "TrackingTools/PatternTools/interface/TrajTrackAssociation.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "CommonTools/UtilAlgos/interface/TFileService.h"
#include "TrackingTools/DetLayers/interface/DetLayer.h"
#include "UserCode/CPEanalyzer/interface/SistripOverlapHit.h"

#include "TH1.h"
#include "TTree.h"

//
// class declaration
//

// If the analyzer does not use TFileService, please remove
// the template argument to the base class so the class inherits
// from  edm::one::EDAnalyzer<>
// This will improve performance in multithreaded jobs.


using reco::TrackCollection;
typedef std::vector<Trajectory> TrajectoryCollection;

struct CPEtree {
  float x,y,z;
  float distance, mdistance, shift, offsetA, offsetB, angle;
};

struct TrackTree {
  float px,py,pz,pt,eta,phi,charge;
};

class CPEanalyzer : public edm::one::EDAnalyzer<edm::one::SharedResources>  {
   public:
      explicit CPEanalyzer(const edm::ParameterSet&);
      ~CPEanalyzer();

      static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);


   private:
      virtual void beginJob() override;
      virtual void analyze(const edm::Event&, const edm::EventSetup&) override;
      virtual void endJob() override;

      // ----------member data ---------------------------
      edm::EDGetTokenT<TrackCollection> tracksToken_;  //used to select what tracks to read from configuration file
      edm::EDGetTokenT<TrajectoryCollection> trajsToken_;  //used to select what trajectories to read from configuration file
      edm::EDGetTokenT<TrajTrackAssociationCollection> tjTagToken_; //association map between tracks and trajectories
      TTree* tree_;
      CPEtree treeBranches_;
      TrackTree trackBranches_;
      
};

//
// constants, enums and typedefs
//

//
// static data member definitions
//

//
// constructors and destructor
//
CPEanalyzer::CPEanalyzer(const edm::ParameterSet& iConfig)
 :
  tracksToken_(consumes<TrackCollection>(iConfig.getUntrackedParameter<edm::InputTag>("tracks"))),
  trajsToken_(consumes<TrajectoryCollection>(iConfig.getUntrackedParameter<edm::InputTag>("trajectories"))),
  tjTagToken_(consumes<TrajTrackAssociationCollection>(iConfig.getUntrackedParameter<edm::InputTag>("association")))

{
   //now do what ever initialization is needed
   usesResource("TFileService");
   edm::Service<TFileService> fs;
   //histo = fs->make<TH1I>("charge" , "Charges" , 3 , -1 , 2 );
   tree_ = fs->make<TTree>("CPEanalysis","CPE analysis tree");
   tree_->Branch("Overlaps", &treeBranches_,"x:y:z:distance:mdistance:shift:offsetA:offsetB:angle");
   tree_->Branch("Tracks", &trackBranches_,"px:py:pz:pt:eta:phi:charge");
}


CPEanalyzer::~CPEanalyzer()
{

   // do anything here that needs to be done at desctruction time
   // (e.g. close files, deallocate resources etc.)

}


//
// member functions
//

// ------------ method called for each event  ------------
void
CPEanalyzer::analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup)
{
   using namespace edm;
   
   std::vector<SiStripOverlapHit> hitpairs;
     
   // loop on trajectories and tracks
   for(const auto& tt : iEvent.get(tjTagToken_) ) {
     auto& traj = *(tt.key);
     auto& track = *(tt.val);
     // track quantities
     trackBranches_.px = track.px();
     trackBranches_.py = track.py();
     trackBranches_.pz = track.pz();
     trackBranches_.pt = track.pt();
     trackBranches_.eta= track.eta();
     trackBranches_.phi= track.phi();
     trackBranches_.charge= track.charge();
     // loop on measurements
     for (auto it = traj.measurements().begin(); it != traj.measurements().end();  ++it ) {
       auto& meas = *it;
       
       // only OT measurements on valid hits (not glued det)
       if(meas.recHit()->geographicalId().subdetId()<3) continue; // (not IT)
       if((meas.recHit()->rawId()&0x3)==0) continue; // (not glued det)
       if(meas.recHit()->getType()!=0) continue; // (only valid hits)
       
       // restrict the search for compatible hits in the same layer (measurements are sorted by layer)
       auto layerRangeEnd = it+1;
       for(; layerRangeEnd<traj.measurements().end(); ++layerRangeEnd) { 
         if(layerRangeEnd->layer()->seqNum() != meas.layer()->seqNum()) break;
       }
       
       // now look for a matching hit in that range.
       auto meas2it = std::find_if(it+1,layerRangeEnd,[meas](const TrajectoryMeasurement & m) -> bool { return (m.recHit()->rawId()&0x3) == (meas.recHit()->rawId()&0x3); });
       
       // check if we found something, build a pair object and add to vector
       if(meas2it != layerRangeEnd) {
         auto& meas2 = *meas2it;
         hitpairs.push_back(SiStripOverlapHit(meas,meas2));
       }
     }
   }

   // some track quantities...
   
   // At this stage we will have a vector of pairs of measurement. Fill a ntuple and some histograms.
   for(const auto& pair : hitpairs) {
     treeBranches_.x = pair.position().x();
     treeBranches_.y = pair.position().y();
     treeBranches_.z = pair.position().z();
     treeBranches_.distance = pair.distance(false);
     treeBranches_.mdistance = pair.distance(true);
     treeBranches_.shift = pair.shift();
     treeBranches_.offsetA = pair.offset(0);
     treeBranches_.offsetB = pair.offset(1);
     treeBranches_.angle = pair.getTrackLocalAngle(0);
     tree_->Fill();
   }
   
   
#ifdef THIS_IS_AN_EVENTSETUP_EXAMPLE
   ESHandle<SetupData> pSetup;
   iSetup.get<SetupRecord>().get(pSetup);
#endif
}


// ------------ method called once each job just before starting event loop  ------------
void
CPEanalyzer::beginJob()
{
}

// ------------ method called once each job just after ending the event loop  ------------
void
CPEanalyzer::endJob()
{
}

// ------------ method fills 'descriptions' with the allowed parameters for the module  ------------
void
CPEanalyzer::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  //The following says we do not know what parameters are allowed so do no validation
  // Please change this to state exactly what you do use, even if it is no parameters
  edm::ParameterSetDescription desc;
  desc.setUnknown();
  descriptions.addDefault(desc);

  //Specify that only 'tracks' is allowed
  //To use, remove the default given above and uncomment below
  //ParameterSetDescription desc;
  //desc.addUntracked<edm::InputTag>("tracks","ctfWithMaterialTracks");
  //descriptions.addDefault(desc);
}

//define this as a plug-in
DEFINE_FWK_MODULE(CPEanalyzer);
