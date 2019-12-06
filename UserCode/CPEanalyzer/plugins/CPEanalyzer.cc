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
#include <algorithm> 

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
#include "DataFormats/Common/interface/DetSetVector.h"
#include "DataFormats/SiStripCluster/interface/SiStripCluster.h"
#include "RecoLocalTracker/Records/interface/TkStripCPERecord.h"
#include "RecoLocalTracker/ClusterParameterEstimator/interface/StripClusterParameterEstimator.h"
#include "Geometry/TrackerGeometryBuilder/interface/StripGeomDetUnit.h"
#include "Geometry/TrackerGeometryBuilder/interface/TrackerGeometry.h"
#include "Geometry/Records/interface/TrackerTopologyRcd.h"
#include "DataFormats/TrackerCommon/interface/TrackerTopology.h"

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

struct CPEBranch {
  float x,y,z;
  float distance, mdistance, shift, offsetA, offsetB, angle;
  float x1r,x2r,x1m,x2m,y1m,y2m;
};

struct TrackBranch {
  float px,py,pz,pt,eta,phi,charge;
};

struct GeometryBranch {
  int subdet, moduleGeometry, stereo, layer, side, ring;
  float pitch;
  int detid;
};

struct ClusterBranch {
  int strips[11];
  int size, firstStrip;
  float barycenter;
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
      
      static bool goodMeasurement(TrajectoryMeasurement const& m);

      // ----------member data ---------------------------
      edm::EDGetTokenT<TrackCollection> tracksToken_;  //used to select what tracks to read from configuration file
      edm::EDGetTokenT<TrajectoryCollection> trajsToken_;  //used to select what trajectories to read from configuration file
      edm::EDGetTokenT<TrajTrackAssociationCollection> tjTagToken_; //association map between tracks and trajectories
      edm::EDGetTokenT<edmNew::DetSetVector<SiStripCluster> > clusToken_; //clusters
      edm::ESInputTag cpeTag_;
      edm::ESHandle<StripClusterParameterEstimator> parameterestimator_; //CPE
      edm::ESHandle<TrackerGeometry> tracker_; //tracker geometry
      edm::ESHandle<TrackerTopology> topology_; //tracker topology
      TTree* tree_;
      CPEBranch treeBranches_;
      TrackBranch trackBranches_;
      GeometryBranch geom1Branches_;
      GeometryBranch geom2Branches_;
      ClusterBranch cluster1Branches_;
      ClusterBranch cluster2Branches_;
      
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
  tjTagToken_(consumes<TrajTrackAssociationCollection>(iConfig.getUntrackedParameter<edm::InputTag>("association"))),
  clusToken_(consumes<edmNew::DetSetVector<SiStripCluster> >(iConfig.getUntrackedParameter<edm::InputTag>("clusters"))),
  cpeTag_(iConfig.getParameter<edm::ESInputTag>("StripCPE"))
{
   //now do what ever initialization is needed
   usesResource("TFileService");
   edm::Service<TFileService> fs;
   //histo = fs->make<TH1I>("charge" , "Charges" , 3 , -1 , 2 );
   tree_ = fs->make<TTree>("CPEanalysis","CPE analysis tree");
   tree_->Branch("Overlaps", &treeBranches_,"x:y:z:distance:mdistance:shift:offsetA:offsetB:angle:x1r:x2r:x1m:x2m:y1m:y2m");
   tree_->Branch("Tracks", &trackBranches_,"px:py:pz:pt:eta:phi:charge");
   tree_->Branch("Cluster1", &cluster1Branches_,"strips[11]/I:size/I:firstStrip/I:barycenter/F");
   tree_->Branch("Cluster2", &cluster2Branches_,"strips[11]/I:size/I:firstStrip/I:barycenter/F");
   tree_->Branch("Geom1", &geom1Branches_, "subdet/I:moduleGeometry/I:stereo/I:layer/I:side/I:ring/I:pitch/F:detid/I");
   tree_->Branch("Geom2", &geom2Branches_, "subdet/I:moduleGeometry/I:stereo/I:layer/I:side/I:ring/I:pitch/F:detid/I");
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
   
   // load the CPE enfine and geometry
   iSetup.get<TkStripCPERecord>().get(cpeTag_, parameterestimator_);
   iSetup.get<TrackerDigiGeometryRecord>().get(tracker_);
   
   // prepare the output   
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
       if(!goodMeasurement(meas)) continue;
       
       // restrict the search for compatible hits in the same layer (measurements are sorted by layer)
       auto layerRangeEnd = it+1;
       for(; layerRangeEnd<traj.measurements().end(); ++layerRangeEnd) { 
         if(layerRangeEnd->layer()->seqNum() != meas.layer()->seqNum()) break;
       }

       // now look for a matching hit in that range.
       auto meas2it = std::find_if(it+1,layerRangeEnd,[meas](const TrajectoryMeasurement & m) -> bool { return goodMeasurement(m) && (m.recHit()->rawId()&0x3) == (meas.recHit()->rawId()&0x3); });

       // check if we found something, build a pair object and add to vector
       if(meas2it != layerRangeEnd) {
         auto& meas2 = *meas2it;
         hitpairs.push_back(SiStripOverlapHit(meas,meas2));
       }  
     }
   }
   
   // load clusters
   Handle<edmNew::DetSetVector<SiStripCluster> > clusters;
   iEvent.getByToken(clusToken_, clusters);

   // At this stage we will have a vector of pairs of measurement. Fill a ntuple and some histograms.
   for(const auto& pair : hitpairs) {
     
     //TODO basically everything below is done twice. -> can be factorized.
     
     // store generic information about the pair
     treeBranches_.x = pair.position().x();
     treeBranches_.y = pair.position().y();
     treeBranches_.z = pair.position().z();
     treeBranches_.distance = pair.distance(false);
     treeBranches_.mdistance = pair.distance(true);
     treeBranches_.shift = pair.shift();
     treeBranches_.offsetA = pair.offset(0);
     treeBranches_.offsetB = pair.offset(1);
     treeBranches_.angle = pair.getTrackLocalAngle(0);
     treeBranches_.x1r = pair.hitA()->localPosition().x();
     treeBranches_.x1m = pair.trajectoryStateOnSurface(0,false).localPosition().x();
     treeBranches_.y1m = pair.trajectoryStateOnSurface(0,false).localPosition().y();
     treeBranches_.x2r = pair.hitB()->localPosition().x();
     treeBranches_.x2m = pair.trajectoryStateOnSurface(1,false).localPosition().x();
     treeBranches_.y2m = pair.trajectoryStateOnSurface(1,false).localPosition().y();
     
     // load the clusters for the detectors
     auto detset1 = (*clusters)[pair.hitA()->rawId()];
     auto detset2 = (*clusters)[pair.hitB()->rawId()];
     
     // look for the proper cluster
     const GeomDetUnit* du;
     du = tracker_->idToDetUnit(pair.hitA()->rawId());
     auto cluster1 = std::min_element(detset1.begin(),detset1.end(),[du,this](SiStripCluster const& cl1,  SiStripCluster const& cl2) { 
       return (fabs( parameterestimator_->localParameters(cl1, *du).first.x() - treeBranches_.x1r) <
               fabs( parameterestimator_->localParameters(cl2, *du).first.x() - treeBranches_.x1r) );
     });
     du = tracker_->idToDetUnit(pair.hitB()->rawId());
     auto cluster2 = std::min_element(detset2.begin(),detset2.end(),[du,this](SiStripCluster const& cl1,  SiStripCluster const& cl2) { 
       return (fabs( parameterestimator_->localParameters(cl1, *du).first.x() - treeBranches_.x2r) <
               fabs( parameterestimator_->localParameters(cl2, *du).first.x() - treeBranches_.x2r) );
     });
     
     // store the amplitudes centered around the maximum
     auto amplitudes1 = cluster1->amplitudes();
     auto amplitudes2 = cluster2->amplitudes();
     auto max1 = std::max_element(amplitudes1.begin(),amplitudes1.end());
     auto max2 = std::max_element(amplitudes2.begin(),amplitudes2.end());
     for(unsigned int i=0;i<11;++i) {
       cluster1Branches_.strips[i]=0.;
       cluster2Branches_.strips[i]=0.;
     }
     cluster1Branches_.size = amplitudes1.size();
     cluster2Branches_.size = amplitudes2.size();
     cluster1Branches_.firstStrip = cluster1->firstStrip();
     cluster1Branches_.barycenter = cluster1->barycenter();
     cluster2Branches_.firstStrip = cluster2->firstStrip();
     cluster2Branches_.barycenter = cluster2->barycenter();

     int cnt = 0;
     int offset = 5-(max1-amplitudes1.begin());
     for(auto& s : amplitudes1) {
       if((offset+cnt)>=0 && (offset+cnt)<11) {
         cluster1Branches_.strips[offset+cnt] = s;
       }
       cnt++;
     }
     cnt = 0;
     offset = 5-(max2-amplitudes2.begin());
     for(auto& s : amplitudes2) {
       if((offset+cnt)>=0 && (offset+cnt)<11) {
         cluster2Branches_.strips[offset+cnt] = s;
       }
       cnt++;
     }

     // store information about the geometry (for both sensors)
     iSetup.get<TrackerTopologyRcd>().get(topology_);
     const TrackerTopology* const tTopo = topology_.product();
     DetId detid1 = pair.hitA()->geographicalId();
     DetId detid2 = pair.hitB()->geographicalId();
     geom1Branches_.detid = detid1.rawId();
     geom2Branches_.detid = detid2.rawId();
     geom1Branches_.subdet = detid1.subdetId();
     geom2Branches_.subdet = detid2.subdetId();
     geom1Branches_.moduleGeometry = tTopo->moduleGeometry(detid1);
     geom2Branches_.moduleGeometry = tTopo->moduleGeometry(detid2);
     geom1Branches_.stereo = tTopo->isStereo(detid1);
     geom2Branches_.stereo = tTopo->isStereo(detid2);
     geom1Branches_.layer = tTopo->layer(detid1);
     geom2Branches_.layer = tTopo->layer(detid2);
     geom1Branches_.side = tTopo->side(detid1);
     geom2Branches_.side = tTopo->side(detid2);
     if(geom1Branches_.subdet==StripSubdetector::TID) {
       geom1Branches_.ring = tTopo->tidRing(detid1);
     } else if (geom1Branches_.subdet==StripSubdetector::TEC) {
       geom1Branches_.ring = tTopo->tecRing(detid1);
     } else {
       geom1Branches_.ring = 0;
     }
     if(geom2Branches_.subdet==StripSubdetector::TID) {
       geom2Branches_.ring = tTopo->tidRing(detid2);
     } else if (geom2Branches_.subdet==StripSubdetector::TEC) {
       geom2Branches_.ring = tTopo->tecRing(detid2);
     } else {
       geom2Branches_.ring = 0;
     }

     const StripGeomDetUnit* theStripDet;
     theStripDet = dynamic_cast<const StripGeomDetUnit*>(tracker_->idToDetUnit(pair.hitA()->rawId()));
     geom1Branches_.pitch = theStripDet->specificTopology().localPitch(pair.trajectoryStateOnSurface(0,false).localPosition());
     theStripDet = dynamic_cast<const StripGeomDetUnit*>(tracker_->idToDetUnit(pair.hitB()->rawId()));
     geom2Branches_.pitch = theStripDet->specificTopology().localPitch(pair.trajectoryStateOnSurface(1,false).localPosition()); 
     
     //fill the tree.
     tree_->Fill(); // we fill one entry per overlap, to track info is multiplicated
   }
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

//TODO update this
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

bool CPEanalyzer::goodMeasurement(TrajectoryMeasurement const& m) {
  return m.recHit()->isValid() &&                         // valid
        m.recHit()->geographicalId().subdetId()>2 &&      // not IT
        (m.recHit()->rawId()&0x3)!=0 &&                   // not glued DetLayer
        m.recHit()->getType()==0 ;                        // only valid hits (redundant?)
}

//define this as a plug-in
DEFINE_FWK_MODULE(CPEanalyzer);
