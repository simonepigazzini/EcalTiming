
// system include files
#include <memory>
#include <iostream>
#include <fstream>

// Make Histograms the way!!
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "CommonTools/UtilAlgos/interface/TFileService.h"

// user include files
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/EDConsumerBase.h"

#include "FWCore/Framework/interface/EDProducer.h"
#include "FWCore/Framework/interface/Event.h"
//#include "FWCore/Framework/interface/Handle.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Utilities/interface/InputTag.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"

#include "DataFormats/Common/interface/Handle.h"
//#include "FWCore/Framework/interface/LooperFactory.h"
//#include "FWCore/Framework/interface/ESProducerLooper.h"
#include "FWCore/Framework/interface/EDFilter.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/Framework/interface/ESProducts.h"
#include "FWCore/Framework/interface/Event.h"
// input collections
#include "DataFormats/EcalRecHit/interface/EcalRecHitCollections.h"
//RingTools
#include "Calibration/Tools/interface/EcalRingCalibrationTools.h"
#include "Geometry/CaloGeometry/interface/CaloGeometry.h"
#include "Geometry/CaloGeometry/interface/CaloSubdetectorGeometry.h"
#include "Geometry/Records/interface/CaloGeometryRecord.h"
#include "Geometry/EcalMapping/interface/EcalElectronicsMapping.h"
#include "Geometry/EcalMapping/interface/EcalMappingRcd.h"

// record to be produced:
#include "CondFormats/DataRecord/interface/EcalTimeCalibConstantsRcd.h"
#include "CondFormats/DataRecord/interface/EcalTimeCalibErrorsRcd.h"
#include "CondFormats/DataRecord/interface/EcalTimeOffsetConstantRcd.h"
#include "CondTools/Ecal/interface/EcalTimeCalibConstantsXMLTranslator.h"
#include "CondTools/Ecal/interface/EcalTimeCalibErrorsXMLTranslator.h"
#include "CondTools/Ecal/interface/EcalTimeOffsetXMLTranslator.h"
#include "CondTools/Ecal/interface/EcalCondHeader.h"

#include "CondFormats/EcalObjects/interface/EcalTimeCalibConstants.h"
#include "CondFormats/EcalObjects/interface/EcalTimeCalibErrors.h"
#include "CondFormats/EcalObjects/interface/EcalTimeOffsetConstant.h"

#include "EcalTiming/EcalTiming/interface/EcalTimingEvent.h"
#include "EcalTiming/EcalTiming/interface/EcalCrystalTimingCalibration.h"

#include "DataFormats/EcalDetId/interface/EBDetId.h"
#include "DataFormats/EcalDetId/interface/EEDetId.h"

#include "TSystem.h"
#include "TFile.h"
#include "TProfile.h"
#include "TGraphErrors.h"
#include "TGraph.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TFile.h"
#include "TProfile2D.h"

#include <fstream>
#include <string>
#include <vector>
#include <iostream>
#include <map>
#include <vector>
#include <algorithm>
#include <functional>
#include <set>
#include <assert.h>
#include <time.h>

#include <TMath.h>
#include <Math/VectorUtil.h>
//#include <boost/tokenizer.hpp>

#include "EcalTiming/EcalTiming/interface/EcalTimeCalibrationMapFwd.h"
#include "EcalTiming/EcalTiming/interface/EcalTimeCalibrationUtils.h"

#include "FWCore/FWLite/interface/FWLiteEnabler.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/PythonParameterSet/interface/MakeParameterSets.h"
#include "PhysicsTools/Utilities/macros/setTDRStyle.C"

using namespace std;

int main(int argc, char** argv)
{

    gSystem->Load( "libFWCoreFWLite" );
    FWLiteEnabler::enable();

    if( argc < 2 ) {
        cout << "Usage : " << argv[0] << " [parameters.py]" << endl;
        return 0;
    }
    if( !edm::readPSetsFrom( argv[1] )->existsAs<edm::ParameterSet>( "process" ) ) {
        cout << " ERROR: ParametersSet 'process' is missing in your configuration file"
             << endl;
        return 0;
   }

   const edm::ParameterSet &process = edm::readPSetsFrom( argv[1] )->getParameter<edm::ParameterSet>( "process" );
   const edm::ParameterSet &filesOpt = process.getParameter<edm::ParameterSet>( "ioFilesOpt" );
   const edm::ParameterSet &calibOpt = process.getParameter<edm::ParameterSet>( "calibOpt" );

   string outputDir = filesOpt.getParameter<string>( "outputDir" );
   if( outputDir == "" ) outputDir = "$CMSSW_BASE/src/EcalTiming/EcalTiming/output/"; 
   string outputCalib = filesOpt.getParameter<string>( "outputCalib" );
   if( outputCalib == "" ) outputCalib = "ecalTiming.dat"; 
   string outputCalibCorr = filesOpt.getParameter<string>( "outputCalibCorr" );
   if( outputCalibCorr == "" ) outputCalibCorr = "ecalTiming-corr.dat"; 
   string outputFile = filesOpt.getParameter<string>( "outputFile" );
   if( outputFile == "" ) outputFile = "ecalTiming.root"; 

   string inputFile = filesOpt.getParameter<string>( "inputFile" );
   string inputTree = filesOpt.getParameter<string>( "inputTree" );
   TFile* inFile = TFile::Open(inputFile.c_str());
   TTree* tree = (TTree*)inFile->Get(inputTree.c_str());

   int nSigma = calibOpt.getParameter<int>( "nSigma" );
   int maxRange = calibOpt.getParameter<int>( "maxRange" );

   TProfile2D* EneMapEB_ = new TProfile2D("EneMapEB", "RecHit Energy[GeV] EB profile map;i#phi; i#eta;E[GeV]", 360, 1., 361., 171, -85, 86);
   TProfile2D* TimeMapEB_ = new TProfile2D("TimeMapEB", "Mean Time [ns] EB profile map; i#phi; i#eta;Time[ns]", 360, 1., 361., 171, -85, 86);
   TProfile2D* TimeErrorMapEB_ = new TProfile2D("TimeErrorMapEB", "Error Time [ns] EB profile map; i#phi i#eta;Time[ns]", 361, 1., 361., 171, -85, 86);

   TProfile2D* EneMapEEP_ = new TProfile2D("EneMapEEP", "RecHit Energy[GeV] profile map EE+;ix;iy;E[GeV]", 100, 1, 101, 100, 1, 101);
   TProfile2D* EneMapEEM_ = new TProfile2D("EneMapEEM", "RecHit Energy[GeV] profile map EE-;ix;iy;E[GeV]", 100, 1, 101, 100, 1, 101);
   TProfile2D* TimeMapEEP_ = new TProfile2D("TimeMapEEP", "Mean Time[ns] profile map EE+;ix;iy; Time[ns]", 100, 1, 101, 100, 1, 101);
   TProfile2D* TimeMapEEM_ = new TProfile2D("TimeMapEEM", "Mean Time[ns] profile map EE-;ix;iy; Time[ns]", 100, 1, 101, 100, 1, 101);
   TProfile2D* TimeErrorMapEEP_ = new TProfile2D("TimeErrorMapEEP", "Error Time[ns] profile map EE+;ix;iy; Time[ns]", 100, 1, 101, 100, 1, 101);
   TProfile2D* TimeErrorMapEEM_ = new TProfile2D("TimeErrorMapEEM", "Error Time[ns] profile map EE-;ix;iy; Time[ns]", 100, 1, 101, 100, 1, 101);

   TH1F* RechitEneEB_ = new TH1F("RechitEneEB", "RecHit Energy[GeV] EB;Rechit Energy[GeV]; Events", 200, 0.0, 100.0);
   TH1F* RechitTimeEB_ = new TH1F("RechitTimeEB", "RecHit Mean Time[ns] EB;RecHit Time[ns]; Events", 200, -50.0, 50.0);
   TH1F* RechitEneEEM_ = new TH1F("RechitEneEEM", "RecHit Energy[GeV] EE-;Rechit Energy[GeV]; Events", 200, 0.0, 100.0);
   TH1F* RechitTimeEEM_ = new TH1F("RechitTimeEEM", "RecHit Mean Time[ns] EE-;RecHit Time[ns]; Events", 200, -50.0, 50.0);
   TH1F* RechitEneEEP_ = new TH1F("RechitEneEEP", "RecHit Energy[GeV] EE+;Rechit Energy[GeV]; Events", 200, 0.0, 100.0);
   TH1F* RechitTimeEEP_ = new TH1F("RechitTimeEEP", "RecHit Mean Time[ns] EE+;RecHit Time[ns]; Events", 200, -50.0, 50.0);

   TProfile2D* HWTimeMapEB_ = new TProfile2D("HWTimeMapEB",  "Mean HW Time[ns] EB profile map; i#phi; i#eta;Time[ns]", 360, 1., 361., 171, -85, 86);
   TProfile2D* HWTimeMapEEM_ = new TProfile2D("HWTimeMapEEM", "Mean HW Time[ns] profile map EE-;ix;iy; Time[ns]", 100, 1, 101, 100, 1, 101);
   TProfile2D* HWTimeMapEEP_ = new TProfile2D("HWTimeMapEEP", "Mean HW Time[ns] profile map EE+;ix;iy; Time[ns]", 100, 1, 101, 100, 1, 101);

   TProfile2D* OccupancyEB_ = new TProfile2D("OccupancyEB", "Occupancy EB; i#phi; i#eta; #Hits", 360, 1., 361., 171, -85, 86);
   TProfile2D* OccupancyEEM_ = new TProfile2D("OccupancyEEM", "OccupancyEEM; iy; ix; #Hits", 100, 1, 101, 100, 1, 101);
   TProfile2D* OccupancyEEP_ = new TProfile2D("OccupancyEEP", "OccupancyEEP; iy; ix; #Hits", 100, 1, 101, 100, 1, 101);


   map< int,map< int, map< int,uint32_t > > > rawIDMap;
   map< int,map< int, map< int,UShort_t > > > elecIDMap;
   map< uint32_t,int > numMap;
   map< uint32_t,float  > sigmaMap;
   map< uint32_t,float > meanEMap;

   map< int,map< int, map< int,vector<float> > > > timingEventsMap_time;
   map< int,map< int, map< int,vector<float> > > > timingEventsMap_energy;
   map< int,vector<float> > timingEventsHWMap_time;
   map< int,vector<float> > timingEventsRingMap_time;

   // Declaration of leaf types
   UInt_t        rawid;
   Int_t         ix;
   Int_t         iy;
   Int_t         iz;
   Float_t       time;
   Float_t       energy;
   UShort_t      elecID;
   Int_t         iRing;

   // List of branches
   TBranch        *b_rawid;   //!
   TBranch        *b_ix;   //!
   TBranch        *b_iy;   //!
   TBranch        *b_iz;   //!
   TBranch        *b_time;   //!
   TBranch        *b_energy;   //!
   TBranch        *b_elecID;   //!
   TBranch        *b_iRing;   //!
   
   tree->SetBranchAddress("rawid", &rawid, &b_rawid);
   tree->SetBranchAddress("ix", &ix, &b_ix);
   tree->SetBranchAddress("iy", &iy, &b_iy);
   tree->SetBranchAddress("iz", &iz, &b_iz);
   tree->SetBranchAddress("time", &time, &b_time);
   tree->SetBranchAddress("energy", &energy, &b_energy);
   tree->SetBranchAddress("elecID", &elecID, &b_elecID);
   tree->SetBranchAddress("iRing", &iRing, &b_iRing);

   for(int entry = 0; entry < tree->GetEntries(); entry++){

        if(entry%1000000==0) std::cout << "--- Reading entry = " << entry << std::endl;
        tree->GetEntry(entry);

        if(iz == 0) ix += 85;
        else if(iz < 0) iz = 1;
        else if(iz > 0)iz = 2;

        rawIDMap[ix][iy][iz] = rawid;
        elecIDMap[ix][iy][iz] = elecID;
        timingEventsMap_time[ix][iy][iz].push_back(time); 
        timingEventsMap_energy[ix][iy][iz].push_back(energy);
        timingEventsHWMap_time[elecID].push_back(time);
        timingEventsRingMap_time[iRing].push_back(time);

   }

   EcalTimeCalibrationUtils* untils = new EcalTimeCalibrationUtils();
   EcalTimeCalibConstants timeCalibConstants;

   for(auto tx : timingEventsMap_time) 
      for(auto ty : tx.second) 
          for(auto tz : ty.second) {

              untils->clear();
              untils->add(timingEventsMap_time[tx.first][ty.first][tz.first]); 
              numMap[rawIDMap[tx.first][ty.first][tz.first]] = untils->num();
              sigmaMap[rawIDMap[tx.first][ty.first][tz.first]] = untils->stdDev();

              if(untils->num() == 0) continue; 

              int ieta = 0;
              int iphi = 0;
              int ix = 0;    
              int iy = 0;    
              
              if(tz.first == 0) {
                 ieta = tx.first-85;
                 iphi = ty.first;
              } else if(tz.first < 0) {
                 ix = tx.first;
                 iy = ty.first;
              } else if(tz.first > 0) {
                 ix = tx.first;
                 iy = ty.first;
              }

              if(tz.first == 0) {
                 TimeMapEB_->Fill(iphi,ieta, untils->getMeanWithinNSigma(nSigma,maxRange)); 
	         TimeErrorMapEB_->Fill(iphi,ieta, untils->getMeanErrorWithinNSigma(nSigma,maxRange)); 
                 OccupancyEB_->Fill(iphi,ieta, untils->num()); 
                 RechitTimeEB_->Fill(untils->getMeanWithinNSigma(nSigma,maxRange)); 
              } else if(tz.first == 1) {
		 TimeMapEEM_->Fill(ix,iy, untils->getMeanWithinNSigma(nSigma,maxRange));
		 TimeErrorMapEEM_->Fill(ix,iy, untils->getMeanErrorWithinNSigma(nSigma,maxRange));
                 OccupancyEEM_->Fill(ix,iy, untils->num());
		 RechitTimeEEM_->Fill(untils->getMeanWithinNSigma(nSigma,maxRange));
	      } else if(tz.first == 2) {
		 TimeMapEEP_->Fill(ix,iy, untils->getMeanWithinNSigma(nSigma,maxRange));
		 TimeErrorMapEEP_->Fill(ix,iy, untils->getMeanErrorWithinNSigma(nSigma,maxRange));
                 OccupancyEEP_->Fill(ix,iy, untils->num());
		 RechitTimeEEP_->Fill(untils->getMeanWithinNSigma(nSigma,maxRange));
              }

              float correction =  -1*untils->getMeanWithinNSigma(nSigma,maxRange);
              timeCalibConstants.setValue(rawIDMap[tx.first][ty.first][tz.first], correction);

              untils->clear();
              untils->add(timingEventsMap_energy[tx.first][ty.first][tz.first]); 
              meanEMap[rawIDMap[tx.first][ty.first][tz.first]] = untils->mean();

              if(tz.first == 0) {
                 EneMapEB_->Fill(iphi,ieta, untils->mean()); 
	         RechitEneEB_->Fill(untils->mean());  
              } else if(tz.first == 1) {
	         EneMapEEM_->Fill(ix,iy, untils->mean());
		 RechitEneEEM_->Fill(untils->mean());
	      } else if(tz.first == 2) {
		 EneMapEEP_->Fill(ix,iy, untils->mean());
		 RechitEneEEP_->Fill(untils->mean());
              }

              untils->clear();
              untils->add(timingEventsHWMap_time[elecIDMap[tx.first][ty.first][tz.first]]); 
              
              if(tz.first == 0) {
                 HWTimeMapEB_->Fill(iphi,ieta, untils->mean());   
              } else if(tz.first == 1) {
	         HWTimeMapEEM_->Fill(ix,iy, untils->mean());
	      } else if(tz.first == 2) {
		 HWTimeMapEEP_->Fill(ix,iy, untils->mean());
              }
                
          }
  
   std::ofstream fout((outputDir+outputCalib).c_str());   
   
   //EB
   for(unsigned int i = 0; i < timeCalibConstants.barrelItems().size(); ++i) {
        EBDetId id(EBDetId::detIdFromDenseIndex(i)); 
	fout << id.ieta() << "\t" << id.iphi() << "\t" << 0 << "\t" << timeCalibConstants.barrelItems()[i] << "\t" << id.rawId() << std::endl;
   }
   //EE
   for(unsigned int i = 0; i < timeCalibConstants.endcapItems().size(); ++i) {
        EEDetId id(EEDetId::detIdFromDenseIndex(i)); 
	fout << id.ix() << "\t" << id.iy() << "\t" << id.zside() << "\t" << timeCalibConstants.endcapItems()[i] << "\t" << id.rawId() << std::endl;
   }

   fout.close();

   std::ofstream fout_corr((outputDir+outputCalibCorr).c_str());

   //EB-
   for(int ieta = -1; ieta>= -85; ieta--)
      for(int iphi = 1; iphi<= 360; iphi++) {
          untils->clear();
          untils->add(timingEventsMap_time[ieta+85][iphi][0]); 
	  if(numMap[rawIDMap[ieta+85][iphi][0]] != 0) fout_corr << ieta << "\t" << iphi << "\t" << 0
			     << "\t" << untils->getMeanWithinNSigma(nSigma,maxRange) << "\t" << sigmaMap[rawIDMap[ieta+85][iphi][0]] << "\t" << numMap[rawIDMap[ieta+85][iphi][0]] << "\t" << meanEMap[rawIDMap[ieta+85][iphi][0]]
			     << "\t" << rawIDMap[ieta+85][iphi][0] << std::endl;
	
      }
   //EB+
   for(int ieta = 1; ieta<= 85; ieta++)
      for(int iphi = 1; iphi<= 360; iphi++) {
          untils->clear();
          untils->add(timingEventsMap_time[ieta+85][iphi][0]); 
	  if(numMap[rawIDMap[ieta+85][iphi][0]] != 0) fout_corr << ieta << "\t" << iphi << "\t" << 0
			     << "\t" << untils->getMeanWithinNSigma(nSigma,maxRange) << "\t" << sigmaMap[rawIDMap[ieta+85][iphi][0]] << "\t" << numMap[rawIDMap[ieta+85][iphi][0]] << "\t" << meanEMap[rawIDMap[ieta+85][iphi][0]]
			     << "\t" << rawIDMap[ieta+85][iphi][0] << std::endl;
	
      }
   //EE-
   for(int ix = 1; ix <=100; ix++)
      for(int iy = 1; iy<= 100; iy++) {
          untils->clear();
          untils->add(timingEventsMap_time[ix][iy][1]); 
	  if(numMap[rawIDMap[ix][iy][1]] != 0) fout_corr << ix << "\t" << iy << "\t" << -1
			     << "\t" << untils->getMeanWithinNSigma(nSigma,maxRange) << "\t" << sigmaMap[rawIDMap[ix][iy][1]] << "\t" << numMap[rawIDMap[ix][iy][1]] << "\t" << meanEMap[rawIDMap[ix][iy][1]]
			     << "\t" << rawIDMap[ix][iy][1] << std::endl;
	
      }
   //EE+
   for(int ix = 1; ix <=100; ix++)
      for(int iy = 1; iy<= 100; iy++) {
          untils->clear();
          untils->add(timingEventsMap_time[ix][iy][2]); 
	  if(numMap[rawIDMap[ix][iy][2]] != 0) fout_corr << ix << "\t" << iy << "\t" << 1
			     << "\t" << untils->getMeanWithinNSigma(nSigma,maxRange) << "\t" << sigmaMap[rawIDMap[ix][iy][2]] << "\t" << numMap[rawIDMap[ix][iy][2]] << "\t" << meanEMap[rawIDMap[ix][iy][2]]
			     << "\t" << rawIDMap[ix][iy][2] << std::endl;
	
      }

   fout_corr.close();
  
   TFile* file = new TFile((outputDir+outputFile).c_str(),"RECREATE");  
   file->cd();

   EneMapEB_->Write();
   TimeMapEB_->Write();
   TimeErrorMapEB_->Write();

   EneMapEEP_->Write();
   EneMapEEM_->Write();
   TimeMapEEP_->Write();
   TimeMapEEM_->Write();
   TimeErrorMapEEP_->Write();
   TimeErrorMapEEM_->Write();

   RechitEneEB_->Write();
   RechitTimeEB_->Write();
   RechitEneEEM_->Write();
   RechitTimeEEM_->Write();
   RechitEneEEP_->Write();
   RechitTimeEEP_->Write();

   HWTimeMapEB_->Write();
   HWTimeMapEEM_->Write();
   HWTimeMapEEP_->Write();

   OccupancyEB_->Write();
   OccupancyEEM_->Write();
   OccupancyEEP_->Write();

   file->Close();
}