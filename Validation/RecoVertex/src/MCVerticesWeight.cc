// -*- C++ -*-
//
// Package:    PileUp
// Class:      MCVerticesWeight
// 
/**\class MCVerticesWeight MCVerticesWeight.cc Validation/RecoVertex/MCVerticesWeight.cc

 Description: 

 Implementation:
     <Notes on implementation>
*/
//
// Original Author:  Andrea Venturi
//         Created:  Tue Oct 21 20:55:22 CEST 2008
//
//


// system include files
#include <memory>
#include <string>

// user include files
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/EDFilter.h"

#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/Framework/interface/ESWatcher.h"

#include "FWCore/ParameterSet/interface/ParameterSet.h"

#include "FWCore/MessageLogger/interface/MessageLogger.h"

#include "FWCore/Utilities/interface/InputTag.h"

#include "SimDataFormats/PileupSummaryInfo/interface/PileupSummaryInfo.h"
#include "SimDataFormats/GeneratorProducts/interface/HepMCProduct.h"
#include "SimDataFormats/GeneratorProducts/interface/GenEventInfoProduct.h"

#include "Validation/RecoVertex/interface/VertexWeighter.h"

//
// class declaration
//

class MCVerticesWeight : public edm::EDFilter {
   public:
      explicit MCVerticesWeight(const edm::ParameterSet&);
      ~MCVerticesWeight();

   private:
      virtual void beginJob() ;
      virtual bool filter(edm::Event&, const edm::EventSetup&);
      virtual void endJob() ;
      
      // ----------member data ---------------------------

  edm::EDGetTokenT< std::vector<PileupSummaryInfo> > m_vecPileupSummaryInfoToken;
  edm::EDGetTokenT< edm::HepMCProduct > m_hepMCProductToken;
  const VertexWeighter m_weighter;

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
MCVerticesWeight::MCVerticesWeight(const edm::ParameterSet& iConfig)
  : m_vecPileupSummaryInfoToken( consumes< std::vector<PileupSummaryInfo> >( iConfig.getParameter< edm::InputTag >( "pileupSummaryCollection" ) ) )
  , m_hepMCProductToken( consumes< edm::HepMCProduct >( iConfig.getParameter< edm::InputTag >( "mcTruthCollection" ) ) )
  , m_weighter( iConfig.getParameter<edm::ParameterSet>( "weighterConfig" ) )
{

  produces<double>();

}

MCVerticesWeight::~MCVerticesWeight()
{
 
   // do anything here that needs to be done at desctruction time
   // (e.g. close files, deallocate resources etc.)

}


//
// member functions
//

// ------------ method called on each new Event  ------------
bool
MCVerticesWeight::filter(edm::Event& iEvent, const edm::EventSetup& iSetup)
{
  
   bool selected = true;
   
   double computed_weight(1);
   
   edm::Handle<std::vector<PileupSummaryInfo> > pileupinfos;
   iEvent.getByToken( m_vecPileupSummaryInfoToken, pileupinfos );


  // look for the intime PileupSummaryInfo

   std::vector<PileupSummaryInfo>::const_iterator pileupinfo;
   for(pileupinfo = pileupinfos->begin(); pileupinfo != pileupinfos->end() ; ++pileupinfo) {
     if(pileupinfo->getBunchCrossing()==0) break;
   } 
   
   //
   if(pileupinfo->getBunchCrossing()!=0) {
     edm::LogError("NoInTimePileUpInfo") << "Cannot find the in-time pileup info " << pileupinfo->getBunchCrossing();
   }
   else {
     
     //     pileupinfo->getPU_NumInteractions();
     
     const std::vector<float>& zpositions = pileupinfo->getPU_zpositions();
     
     //     for(std::vector<float>::const_iterator zpos = zpositions.begin() ; zpos != zpositions.end() ; ++zpos) {
       
     //     }
     
     // main interaction part
     
     edm::Handle< edm::HepMCProduct > EvtHandle ;
     iEvent.getByToken( m_hepMCProductToken, EvtHandle );
     
     const HepMC::GenEvent* Evt = EvtHandle->GetEvent();
     
     // get the first vertex
     
     double zmain = 0.0;
     if(Evt->vertices_begin() != Evt->vertices_end()) {
       zmain = (*Evt->vertices_begin())->point3d().z()/10.;
     }
     
     //
    
     
     computed_weight = m_weighter.weight(zpositions,zmain);
     
   }
   
   std::auto_ptr<double> weight(new double(computed_weight));
   
   iEvent.put(weight);
   
   //
   
  return selected;
}

// ------------ method called once each job just before starting event loop  ------------
void 
MCVerticesWeight::beginJob()
{
}

// ------------ method called once each job just after ending the event loop  ------------
void 
MCVerticesWeight::endJob() {
}

//define this as a plug-in
DEFINE_FWK_MODULE(MCVerticesWeight);
