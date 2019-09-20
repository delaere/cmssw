import FWCore.ParameterSet.Config as cms

process = cms.Process("CPEana")

# initialize MessageLogger and output report
process.load("FWCore.MessageLogger.MessageLogger_cfi")
process.MessageLogger.cerr.threshold = 'INFO'
process.MessageLogger.categories.append('CPEana')
process.MessageLogger.cerr.INFO = cms.untracked.PSet(
        limit = cms.untracked.int32(-1)
        )
process.options   = cms.untracked.PSet( wantSummary = cms.untracked.bool(True) )

process.maxEvents = cms.untracked.PSet( input = cms.untracked.int32(-1) )

process.source = cms.Source("PoolSource", fileNames = cms.untracked.vstring(
  '/store/express/Run2018D/StreamExpress/ALCARECO/SiStripCalMinBias-Express-v1/000/321/305/00001/FE4CA4CC-77A0-E811-A85E-FA163E0CB594.root'
                                                                           )
                           )

process.demo = cms.EDAnalyzer('CPEanalyzer',
                                         minTracks=cms.untracked.uint32(0),
                                         tracks = cms.untracked.InputTag("ALCARECOSiStripCalMinBias","")
                             )

process.TFileService = cms.Service("TFileService",
                                       fileName = cms.string('histodemo.root')
                                  )

process.p = cms.Path(process.demo)
