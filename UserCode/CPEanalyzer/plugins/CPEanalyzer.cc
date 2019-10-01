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
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "CommonTools/UtilAlgos/interface/TFileService.h"
#include "TH1.h"
#include "TrackingTools/DetLayers/interface/DetLayer.h"
//
// class declaration
//

// If the analyzer does not use TFileService, please remove
// the template argument to the base class so the class inherits
// from  edm::one::EDAnalyzer<>
// This will improve performance in multithreaded jobs.


using reco::TrackCollection;
typedef std::vector<Trajectory> TrajectoryCollection;

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
      TH1I * histo;
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
  trajsToken_(consumes<TrajectoryCollection>(iConfig.getUntrackedParameter<edm::InputTag>("trajectories")))

{
   //now do what ever initialization is needed
   usesResource("TFileService");
   edm::Service<TFileService> fs;
   histo = fs->make<TH1I>("charge" , "Charges" , 3 , -1 , 2 );

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


   // this is just for demo purposes
   /*
   for(const auto& track : iEvent.get(tracksToken_) ) {
      // do something with track parameters, e.g, plot the charge.
      // int charge = track.charge();
       histo->Fill( track.charge() );
   }

   for(const auto& traj : iEvent.get(trajsToken_) ) {
      // do something with track parameters, e.g, plot the charge.
      // int charge = track.charge();
      std::cout << traj.foundHits() << std::endl;
   }
   */
   
   // Find pairs of measurements from overlaps
   // - same layer
   // - same module type
   
   // we can use std::find_if(), but is that the best since measurements are sorted by layer already...
   // we could use std::equal_range or std::upper_bound.
   // for an element -> upper bound -> range in which to search for another module of same type (detid&0x3 == 1 or 2)
   
   // loop on trajectories
   for(const auto& traj : iEvent.get(trajsToken_) ) {
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
       
       // check if we found something
       if(meas2it != layerRangeEnd) {
         auto& meas2 = *meas2it;
         std::cout << "FOUND A PAIR: ";
         std::cout << meas.layer()->seqNum() << " " << meas.recHit()->geographicalId().subdetId() << " " << (meas.recHit()->rawId()&0x3) << " ";
         std::cout << meas2.layer()->seqNum() << " " << meas2.recHit()->geographicalId().subdetId() << " " << (meas2.recHit()->rawId()&0x3) << std::endl;
         // TODO build a pair object and add to vector
       }
       
     }
     
     std::cout << "---" << std::endl;
   }

   // TODO at this stage we will have a vector of pairs of measurement. Fill a ntuple and some histograms.
   // measuring the distance, etc. should be done in the pair object.
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
