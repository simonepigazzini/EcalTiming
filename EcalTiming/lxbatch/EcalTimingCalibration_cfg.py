import FWCore.ParameterSet.Config as cms

process = cms.PSet()

process.ioFilesOpt = cms.PSet(

    ##input file
    inputFile = cms.string('root://eoscms.cern.ch//store/user/bmarzocc/ECAL_Timing/Run/300576/ecalTiming.root'),
    
    ##input tree
    inputTree = cms.string('/timing/EcalSplashTiming/timingEventsTree'),

    ## base output directory: default output/
    outputDir = cms.string(''),

    ## base output: default ecalTiming.dat
    outputCalib = cms.string('ecalTiming.dat'),
     
    ## base output: default ecalTiming-corr.dat
    outputCalibCorr = cms.string('ecalTiming-corr.dat'),

    ## base output: default ecalTiming.root
    outputFile = cms.string('ecalTiming.root'),

    ## maxEvents
    maxEvents = cms.untracked.int32(1000000)
)

process.calibOpt = cms.PSet(

    ## nSigma
    nSigma = cms.int32(2),

    ## maxRange
    maxRange = cms.int32(10)
    
)

