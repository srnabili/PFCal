//
#include<string>
#include<iostream>
#include<fstream>
#include<sstream>
#include<map>
#include <boost/algorithm/string.hpp>
#include "boost/lexical_cast.hpp"
#include "boost/program_options.hpp"
#include "boost/format.hpp"
#include "boost/function.hpp"

#include "TFile.h"
#include "TTree.h"
#include "TChain.h"
#include "TH3F.h"
#include "TH2F.h"
#include "TH1F.h"
#include "TF1.h"
#include "TStyle.h"
#include "TCanvas.h"
#include "TLatex.h"
#include "TRandom3.h"
#include "TMatrixD.h"
#include "TMatrixDSym.h"
#include "TVectorD.h"

#include "HGCSSEvent.hh"
#include "HGCSSInfo.hh"
#include "HGCSSSamplingSection.hh"
#include "HGCSSSimHit.hh"
#include "HGCSSRecoHit.hh"
#include "HGCSSGenParticle.hh"
#include "HGCSSParameters.hh"
#include "HGCSSCalibration.hh"
#include "HGCSSDigitisation.hh"
#include "HGCSSDetector.hh"
#include "HGCSSGeometryConversion.hh"
#include "HGCSSPUenergy.hh"
//#include "HGCSSSimpleHit.hh"

#include "PositionFit.hh"
#include "SignalRegion.hh"

#include "Math/Vector3D.h"
#include "Math/Vector3Dfwd.h"
#include "Math/Point2D.h"
#include "Math/Point2Dfwd.h"

#include "utilities.h"

using boost::lexical_cast;
namespace po=boost::program_options;

bool domap = true;


void SNAPrec_rl(TH2F* h_1,std::vector<HGCSSRecoHit> *rechitvec) {
  std::cout<<"SNAP"<<std::endl;
  double ymax = h_1->GetYaxis()->GetXmax();
    for (unsigned iH(0); iH<(*rechitvec).size(); ++iH){//loop on hits
      HGCSSRecoHit lHit = (*rechitvec)[iH];
      double xh=lHit.get_x();
      double yh=lHit.get_y();
      unsigned ilayer=lHit.layer();
      //unsigned ixx=ilayer;
      //if(ixx>52) ixx=ixx-17;
      double rh=sqrt(xh*xh+yh*yh);
      double Eh=lHit.energy();
      h_1->Fill(ilayer+0.5,std::min(rh,ymax-200.),Eh);
    }

  return;
}

void SNAPrec_rz(TH2F* h_1,std::vector<HGCSSRecoHit> *rechitvec) {
  std::cout<<"SNAP"<<std::endl;
  double xmax = h_1->GetXaxis()->GetXmax();
  double ymax = h_1->GetYaxis()->GetXmax();
    for (unsigned iH(0); iH<(*rechitvec).size(); ++iH){//loop on hits
      HGCSSRecoHit lHit = (*rechitvec)[iH];
      double xh=lHit.get_x();
      double yh=lHit.get_y();
      double zh=lHit.get_z();
      double rh=sqrt(xh*xh+yh*yh);
      double Eh=lHit.energy();
      h_1->Fill(std::min(zh,xmax-0.001),std::min(rh,ymax-200.),Eh);
    }

  return;
}

void SNAPsim(TH2F* h_1,std::vector<HGCSSSimHit> *simhitvec,
	     HGCSSDetector & myDetector,
	     HGCSSGeometryConversion & aGeom,
	     unsigned shape
	     ) {
  std::cout<<"SNAP"<<std::endl;
  double xmax = h_1->GetXaxis()->GetXmax();
  double ymax = h_1->GetYaxis()->GetXmax();
    for (unsigned iH(0); iH<(*simhitvec).size(); ++iH){//loop on hits
      HGCSSSimHit lHit = (*simhitvec)[iH];
      unsigned layer = lHit.layer();
      const HGCSSSubDetector & subdet = myDetector.subDetectorByLayer(layer);
      ROOT::Math::XYZPoint pp = lHit.position(subdet,aGeom,shape);
      double Eh=lHit.energy();
      //std::cout<<Eh<<" "<<pp.z()<<" "<<pp.r()<<std::endl;
      h_1->Fill(std::min(pp.z(),xmax-0.001),std::min(pp.r(),ymax-0.001),Eh);
    }

  return;
}



double DeltaR(double eta1,double phi1,double eta2,double phi2){
  double dr=99999.;
  double deta=fabs(eta1-eta2);
  double dphi=fabs(phi1-phi2);
  if(dphi>TMath::Pi()) dphi=2.*TMath::Pi()-dphi;
  dr=sqrt(deta*deta+dphi*dphi);
  return dr;
}


int main(int argc, char** argv){//main  



  ///////////////////////////////////////////////////////////////
  // initialize some variables
  ///////////////////////////////////////////////////////////////


  const unsigned nscintlayer=16;
  const unsigned scintoffset=36;

  bool ipfirst[nscintlayer];
  for(int i(0);i<nscintlayer;i++) {ipfirst[i]=true;}

  unsigned scintminid[nscintlayer]={145,179,219,206,159,205,274,284,28,223,47,24,219,1,188,211};
  unsigned scintmaxid[nscintlayer]={32545,32579,32619,32606,20823,20869,20938,20948,20692,20887,20711,20688,20883,20665,20852,20875};


  //Input output and config options
  std::string cfg;
  unsigned pNevts;
  std::string outFilePath;
  std::string filePath;
  std::string digifilePath;
  unsigned nRuns;
  std::string simFileName;
  std::string recoFileName;
  unsigned debug;
  po::options_description preconfig("Configuration"); 
  preconfig.add_options()("cfg,c",po::value<std::string>(&cfg)->required());
  po::variables_map vm;
  po::store(po::command_line_parser(argc, argv).options(preconfig).allow_unregistered().run(), vm);
  po::notify(vm);
  po::options_description config("Configuration");
  config.add_options()
    //Input output and config options //->required()
    ("pNevts,n",       po::value<unsigned>(&pNevts)->default_value(0))
    ("outFilePath,o",  po::value<std::string>(&outFilePath)->required())
    ("filePath,i",     po::value<std::string>(&filePath)->required())
    ("digifilePath", po::value<std::string>(&digifilePath)->default_value(""))
    ("simFileName,s",  po::value<std::string>(&simFileName)->required())
    ("recoFileName,r", po::value<std::string>(&recoFileName)->required())
    ("nRuns",        po::value<unsigned>(&nRuns)->default_value(0))
    ("debug,d",        po::value<unsigned>(&debug)->default_value(0))
    ;
  po::store(po::command_line_parser(argc, argv).options(config).allow_unregistered().run(), vm);
  po::store(po::parse_config_file<char>(cfg.c_str(), config), vm);
  po::notify(vm);


  std::string inFilePath = filePath+simFileName;

  std::cout << " -- Input parameters: " << std::endl
            << " -- Input file path: " << filePath << std::endl
            << " -- Digi Input file path: " << digifilePath << std::endl
    	    << " -- Output file path: " << outFilePath << std::endl
	    << std::endl
	    << " -- Processing ";
  if (pNevts == 0) std::cout << "all events." << std::endl;
  else std::cout << pNevts << " events per run." << std::endl;

  TRandom3 lRndm(0);
  std::cout << " -- Random number seed: " << lRndm.GetSeed() << std::endl;


  //
  // hardcoded
  //


  //global threshold to reduce size of noise hits
  const double threshMin = 0.5;

  std::cout << " ---- Selection settings: ---- " << std::endl
	    << " -------threshMin " << threshMin << std::endl
	    << " ------------------------------------------" << std::endl;



  /////////////////////////////////////////////////////////////
  //input
  /////////////////////////////////////////////////////////////


  std::ostringstream inputsim;
  inputsim << filePath << "/" << simFileName;
  std::ostringstream inputrec;
  if (digifilePath.size()==0)
    inputrec << filePath << "/" << recoFileName;
  else
    inputrec << digifilePath << "/" << recoFileName;

  std::cout << inputsim.str() << " " << inputrec.str() << std::endl;

  HGCSSInfo * info;

  TChain *lSimTree = new TChain("HGCSSTree");
  TChain *lRecTree = 0;

  TFile * simFile = 0;
  TFile * recFile = 0;

  if (recoFileName.find("Digi") != recoFileName.npos)
    lRecTree = new TChain("RecoTree");
  else lRecTree = new TChain("PUTree");

  if (nRuns == 0){
    if (!testInputFile(inputsim.str(),simFile)) return 1;
    lSimTree->AddFile(inputsim.str().c_str());
    if (simFile) info =(HGCSSInfo*)simFile->Get("Info");
    else {
      std::cout << " -- Error in getting information from simfile!" << std::endl;
      return 1;
    }
    if (!testInputFile(inputrec.str(),recFile)) return 1;
    lRecTree->AddFile(inputrec.str().c_str());
  }
  else {
    for (unsigned i(0);i<nRuns;++i){
      std::ostringstream lstrsim;
      std::ostringstream lstrrec;
      lstrsim << inputsim.str() << "_run" << i << ".root";
      if (testInputFile(lstrsim.str(),simFile)){
        if (simFile) info =(HGCSSInfo*)simFile->Get("Info");
        else {
	  std::cout << " -- Error in getting information from simfile!" << std::endl;
          return 1;
        }
      }
      else continue;
      lstrrec << inputrec.str() << "_run" << i << ".root";
      if (!testInputFile(lstrrec.str(),recFile)) continue;
      lSimTree->AddFile(lstrsim.str().c_str());
      lRecTree->AddFile(lstrrec.str().c_str());
    }
  }

  if (!lSimTree){
    std::cout << " -- Error, tree HGCSSTree cannot be opened. Exiting..." << std::endl;
    return 1;
  }

  if (!lRecTree){
    std::cout << " -- Error, tree RecoTree cannot be opened. Exiting..." << std::endl;
    return 1;
  }





  //assert(info);

  const unsigned versionNumber = info->version();
  const unsigned model = info->model();
  const unsigned shape = info->shape();
  const double cellSize = info->cellSize();
  const double calorSizeXY = info->calorSizeXY();

  bool isTBsetup = (model != 2);
  bool bypassR = false;
  if (isTBsetup) bypassR = true;

  HGCSSDetector & myDetector = theDetector();
  myDetector.buildDetector(versionNumber,true,false,bypassR);

  HGCSSCalibration mycalib(inFilePath);

  //corrected for Si-Scint overlap
  const unsigned nLayers = 52;//


  std::cout << " -- Calor size XY = " << calorSizeXY
	    << ", version number = " << versionNumber 
	    << ", model = " << model << std::endl
	    << " -- cellSize = " << cellSize
	    << ", shape = " << shape
	    << ", nLayers = " << nLayers
	    << std::endl;
  HGCSSGeometryConversion geomConv(model,cellSize,bypassR,3);



  geomConv.setXYwidth(calorSizeXY);
  geomConv.setVersion(versionNumber);
  
  if (shape==2) geomConv.initialiseDiamondMap(calorSizeXY,10.);
  else if (shape==3) geomConv.initialiseTriangleMap(calorSizeXY,10.*sqrt(2.));
  else if (shape==1) geomConv.initialiseHoneyComb(calorSizeXY,cellSize);
  else if (shape==4) geomConv.initialiseSquareMap(calorSizeXY,10.);

  //square map for BHCAL
  geomConv.initialiseSquareMap1(1.4,3.0,-1.*TMath::Pi(),TMath::Pi(),0.01745);//eta phi segmentation
  geomConv.initialiseSquareMap2(1.4,3.0,-1.*TMath::Pi(),TMath::Pi(),0.02182);//eta phi segmentation
  std::vector<unsigned> granularity;
  granularity.resize(myDetector.nLayers(),1);
  geomConv.setGranularity(granularity);
  geomConv.initialiseHistos();

  //////////////////////////////////////////////////
  //////////////////////////////////////////////////
  ///////// Output File // /////////////////////////
  //////////////////////////////////////////////////
  //////////////////////////////////////////////////

  TFile *outputFile = TFile::Open(outFilePath.c_str(),"RECREATE");
  
  if (!outputFile) {
    std::cout << " -- Error, output file " << outFilePath << " cannot be opened. Please create output directory. Exiting..." << std::endl;
    return 1;
  }
  else {
    std::cout << " -- output file " << outputFile->GetName() << " successfully opened." << std::endl;
  }
  outputFile->cd();

  const int nsnap=10;
  TH2F *h_snaprz[nsnap];
  TH2F *h_snaprl[nsnap];
  TH2F *h_snaps[nsnap];
  for(unsigned ii(0);ii<nsnap;ii++) {
    std::ostringstream label;
    label.str("");
    label<<"h_Erlrecosnap_"<<ii;
    h_snaprl[ii]=new TH2F(label.str().c_str(),"E-weighted r-z of reco hit",72,0.,72.,250,0.,3000.);
    std::ostringstream label3;
    label3.str("");
    label3<<"h_Erzrecosnap_"<<ii;
    h_snaprz[ii]=new TH2F(label3.str().c_str(),"E-weighted r-z of reco hit",5000,3100.,5200.,250,0.,3000.);
    std::ostringstream label2;
    label2.str("");
    label2<<"Erzsimsnap_"<<ii;
    h_snaps[ii]=new TH2F(label2.str().c_str(),"E-weighted r-z of sim hit",500,300.,500.,250,250.,700.);

  }

  TH1F* h_energy = new TH1F("h_energy","hit energy",1000,0.,5.);
  TH1F* h_z = new TH1F("h_z","z of hit",5000,3100.,5200);
  TH1F* h_z1 = new TH1F("h_z1","z of hit",5000,3150.,3550);
  TH1F* h_z2 = new TH1F("h_z2","z of hit",5000,3550.,5200);
  TH2F* h_xy = new TH2F("h_xy","xy of hit",1000,-2000,2000,1000,-2000,2000);
  TH1F* h_phibad = new TH1F("h_phibad","gen phi of large neg hits",2000,-3.2,0);
  TH1F* h_xbad = new TH1F("h_xbad","gen x of large neg hits",2000,-2500,2500);
  TH1F* h_ybad = new TH1F("h_ybad","gen y of large neg hits",2000,-2500,2500);
  TH2F* h_xrgood = new TH2F("h_xrgood"," x vs r good",1000,0.,2500.,1000,-2500.,2500.);
  TH2F* h_yrgood = new TH2F("h_yrgood"," y vs r good",1000,0.,2500.,1000,-2500.,2500.);
  TH2F* h_etaphi = new TH2F("h_etaphi","etaphi of hit",1000,1,3.5,1000,-7,7);
  TH2F* h_getaphi = new TH2F("h_getaphi","gen part etaphi of hit",1000,1,3.5,1000,-7,7);
  TH1F* h_l = new TH1F("h_l","layer of hit",80,0.,80.);
  TH1F* h_l2 = new TH1F("h_l2","layer of hit",30,50,80.);
  TH2F* h_zl = new TH2F("h_zl","z vs l of hit",5000,4300.,5200,25,30.,55.);
  TH2F* h_simD1 = new TH2F("h_simD1","energy sim hits in eta phi",100,1.,4.,100,-4.,4.);
  TH2F* h_simD2 = new TH2F("h_simD2","weight sim hits in eta phi",100,1.,4.,100,-4.,4.);
  TH1F* h_simD3 = new TH1F("h_simD3","calib weight",100,0.,100.);
  TH2F* h_simD4 = new TH2F("h_simD4","weight versus layer",100,1.,100.,100,0.,100.);
  TH2F* h_simD5 = new TH2F("h_simD5","weight versus r",100,0.,300.,100,0.,100.);
  TH2F* h_simD6 = new TH2F("h_simD6","reco versus sim ",100,0.,400.,100,0.,1200.);

  TH2F *h_sxy[nscintlayer]; // last 16 layers, scint part
  TH2F *h_nsxy[nscintlayer];  // last 16 layers, not scint part
  TH2F *h_scellid[nscintlayer];
  TH2F *h_scellidxy[nscintlayer];
  TH2F *h_scellidxyzoom[nscintlayer];
  TH2F *h_scellideta[nscintlayer];
  TH2F *h_scellidphi[nscintlayer];
  for(unsigned ii(0);ii<nscintlayer;ii++) {
    std::ostringstream label;
    label.str("");
    label<<"xy of hit scint "<<ii+scintoffset;
    h_sxy[ii]=new TH2F(label.str().c_str(),"xy of hit scint",2500,-2500.,2500.,2500,-2500.,2500.);
    std::ostringstream label2;
    label2.str("");
    label2<<"xy of hit not scint "<<ii+scintoffset;
    h_nsxy[ii]=new TH2F(label2.str().c_str(),"xy of hit not scint",
        2500,-2500.,2500.,2500,-2500.,2500.);
    std::ostringstream label3;
    label3.str("");
    label3<<"cellidscint"<<ii+scintoffset;
    h_scellid[ii]=new TH2F(label3.str().c_str(),"cellid scint",
			   2500,-3.2,3.2,2500,1.3,4.0);  // temp phi pos to Anne-Marie fixes cell geo problem
    std::ostringstream label4;
    label4.str("");
    label4<<"cellidscinteta"<<ii+scintoffset;
    h_scellideta[ii]=new TH2F(label4.str().c_str(),"cellid scint",
			   2500,1.3,3.5,13500,0,13500);
    std::ostringstream label5;
    label5.str("");
    label5<<"cellidscintphi"<<ii+scintoffset;
    h_scellidphi[ii]=new TH2F(label5.str().c_str(),"cellid scint",
			      2500,-3.2,3.2,1000,0,2000);
    std::ostringstream label6;
    label6.str("");
    label6<<"cellidxy"<<ii+scintoffset;
    h_scellidxy[ii]=new TH2F(label6.str().c_str(),"cellid scint",
			   6000,-3000.,3000.,6000,-3000.,3000.);  // temp phi pos to    
    std::ostringstream label7;
    label7.str("");
    label7<<"cellidxyzoom"<<ii+scintoffset;
    h_scellidxyzoom[ii]=new TH2F(label7.str().c_str(),"cellid scint",
			   6000,-700.,700.,6000,-700.,700.);  // temp phi pos to Anne-Mar
  }

  TH2F* h_Egenreco = new TH2F("h_Egenreco","E reco sum versus gen",1000,0.,1000.,100,0.,20.);
  TH1F* h_egenreco = new TH1F("h_egenreco","E reco sum over gen",100,0.,2.);
  TH1F* h_egenrecopphi = new TH1F("h_egenrecopphi","E reco sum over gen plus phi",100,0.,2.);
  TH1F* h_egenrecopphidead = new TH1F("h_egenrecopphidead","E reco sum over gen plus phi",100,0.,2.);
  TH1F* h_egenrecomphi = new TH1F("h_egenrecomphi","E reco sum over gen minus phi",100,0.,2.);


  TH2F* h_EpCone = new TH2F("h_EpCone","Ereco/gen versus cone size",10,0.,1.,100,0.,2.);
  TH2F* h_EpPhi = new TH2F("h_EpPhi","Ereco/gen versus phi",100,-4.,4.,100,0.,2.);
  TH2F* h_etagenmax= new TH2F("h_etagenmax","eta gen vs max",100,1.,5.,100,1.,5.);
  TH2F* h_phigenmax= new TH2F("h_phigenmax","phi gen vs max",100,-4,4.,100,-4.,4.);
  TH1F* h_phimax = new TH1F("h_phimax","phi of max hit",100,-4.,4.);
  TH1F* h_etamax = new TH1F("h_etamax","eta of max hit",100,1.,5.);
  TH1F* h_maxE = new TH1F("h_maxE","energy of highest energy hit",1000,0.,1000.);
  TH1F* h_ECone03 = new TH1F("h_ECone03","Sum energy cone 03",1000,0.,1500.);
  TH1F* h_simHit03 = new TH1F("h_simHit03","Sum simhit energy cone 03",1000,0.,1500.);

  TH2F* h_ssvec = new TH2F("h_ssvec","ssvec versus layer number", 70,0.,70.,100,0.,100.);
  TH1F* h_cellid = new TH1F("h_cellid","cell is",25000,0.,250000.);
  TH1F* h_cellids = new TH1F("h_cellids","max scint sim cell is",250,0.,10.);
  TH1F* h_badcellids = new TH1F("h_badcellids","bad max scint sim cell is",250,0.,10.);
  TH2F* h_cellidz = new TH2F("h_cellidz","cell id versus z",5000,3100,5200,1000,0.,250000.);
  TH2F* h_cellidzs = new TH2F("h_cellidzs","scint cell id versus z exclude bad",5000,3100,5200,13336,0.,13336.);
  TH2F* h_cellidphis = new TH2F("h_cellidphis","scint cell id versus phi exclude bad",5000,-3.2,6.4,13336,0.,13336.);
  TH2F* h_cellidphibs = new TH2F("h_cellidphibs","scint cell id versus phi bad",5000,-3.2,6.4,13336,0.,13336.);
  TH2F* h_cellidetas = new TH2F("h_cellidetas","scint cell id versus eta exclude bad",5000,1.3,3.2,13336,0.,13336.);

  TH2F* h_banana = new TH2F("h_banana","banana plot",1000,0.,500.,1000,0.,500.);
  TH1F* h_fracBH = new TH1F("h_fracBH","fraction in BH",100,-01.,1.1);

  TH1F* h_eWeirdSim = new TH1F("h_eWeirdSim","energy of sim hits with invalid cell id",100,0.,100.);





  //////////////////////////////
  // for missing channel study
  //

  
  //const double deadfrac=0.00003;
  const double deadfrac=0.5;
  std::set<std::pair<unsigned, unsigned>> deadlist;
  unsigned nchan=0;
  for(int i(0);i<nscintlayer;i++) {
    nchan+=(scintmaxid[i]-scintminid[i]);
  }
  std::cout<<"total number scintillator channels is "<<nchan<<std::endl;
  unsigned ndead=deadfrac*nchan;
  unsigned ld;
  unsigned cd;
  unsigned range;

  for(int i(0);i<ndead;i++) {
    ld=lRndm.Integer(nscintlayer);
    range=scintmaxid[ld]-scintminid[ld];
    cd=scintminid[ld]+(lRndm.Integer(range));
    //std::cout<<ld<<" "<<cd<<std::endl;
    deadlist.insert(std::make_pair(ld,cd));    
  }
  /*
  std::cout<<" dead list is "<<std::endl;
  for(auto itr=deadlist.begin();itr!=deadlist.end();itr++ ) {
    std::cout<<(*itr).first<<" "<<(*itr).second<<std::endl;
  }
  */
  
  ///////////////////////////////////////////////////////
  //////////////////  start event loop
  //////////////////////////////////////////////////////

  //  const unsigned nEvts = ((pNevts > lRecTree->GetEntries() || pNevts==0) ? static_cast<unsigned>(lRecTree->GetEntries()) : pNevts) ;
  
  //std::cout << " -- Processing " << nEvts << " events out of " << 
  //   lRecTree->GetEntries()<< std::endl;


  const unsigned nEvts = ((pNevts > lSimTree->GetEntries() || pNevts==0) ? static_cast<unsigned>(lSimTree->GetEntries()) : pNevts) ;

  std::cout << " -- Processing " << nEvts << " events out of " << lSimTree->GetEntries() << " " << lRecTree->GetEntries() << std::endl;


  //loop on events
  HGCSSEvent * event = 0;
  HGCSSEvent * eventRec = 0;
  std::vector<HGCSSSamplingSection> * ssvec = 0;
  std::vector<HGCSSSimHit> * simhitvec = 0;
  std::vector<HGCSSSimHit> * alusimhitvec = 0;
  std::vector<HGCSSRecoHit> * rechitvec = 0;
  std::vector<HGCSSGenParticle> * genvec = 0;
  unsigned nPuVtx = 0;



  lSimTree->SetBranchAddress("HGCSSEvent",&event);
  lSimTree->SetBranchAddress("HGCSSSamplingSectionVec",&ssvec);
  lSimTree->SetBranchAddress("HGCSSSimHitVec",&simhitvec);
  //  lSimTree->SetBranchAddress("HGCSSAluSimHitVec",&alusimhitvec);
  lSimTree->SetBranchAddress("HGCSSGenParticleVec",&genvec);

  lRecTree->SetBranchAddress("HGCSSEvent",&eventRec);
  lRecTree->SetBranchAddress("HGCSSRecoHitVec",&rechitvec);
  if (lRecTree->GetBranch("nPuVtx")) lRecTree->SetBranchAddress("nPuVtx",&nPuVtx);



  unsigned ievtRec = 0;
  unsigned nSkipped = 0;
  std::vector<double> absW;
  bool firstEvent = true;
  bool firstEvent2 = true;
  unsigned ibinScintmin[nscintlayer];
  for(int i(0);i<nscintlayer;i++) {ibinScintmin[i]=4294967295;}
  unsigned ibinScintmax[nscintlayer];
  for(int i(0);i<nscintlayer;i++) {ibinScintmax[i]=0;}
  unsigned badlaymin=10000;
  
  double rmaxs[nscintlayer] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  double rmins[nscintlayer]={5000,5000,5000,5000,5000,5000,5000,5000,5000,5000,5000,5000,5000,5000,5000,5000};
  double rmaxns[nscintlayer] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  double rminns[nscintlayer]={5000,5000,5000,5000,5000,5000,5000,5000,5000,5000,5000,5000,5000,5000,5000,5000};


  double xmax[nscintlayer] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  double xmin[nscintlayer]={5000,5000,5000,5000,5000,5000,5000,5000,5000,5000,5000,5000,5000,5000,5000,5000};
  double ymax[nscintlayer] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  double ymin[nscintlayer]={5000,5000,5000,5000,5000,5000,5000,5000,5000,5000,5000,5000,5000,5000,5000,5000};
  
  int isnap=-1;
  for (unsigned ievt(0); ievt<nEvts; ++ievt){//loop on entries
    if (ievtRec>=lRecTree->GetEntries()) continue;

    mycalib.setVertex(0.,0.,0.);

    if (debug) std::cout << std::endl<<std::endl<<"... Processing entry: " << ievt << std::endl;
    else if (ievt%50 == 0) std::cout << "... Processing entry: " << ievt << std::endl;



    lSimTree->GetEntry(ievt);
    lRecTree->GetEntry(ievtRec);
    if (nPuVtx>0 && eventRec->eventNumber()==0 && event->eventNumber()!=0) {
      std::cout << " skip !" << ievt << " " << ievtRec << std::endl;
      nSkipped++;
      continue;
    }


    if(firstEvent) {
      firstEvent=false;
      std::cout<<" size of ssvec of weights is "<<(*ssvec).size()<<std::endl;
      double absweight=0;      
      for (unsigned iL(0); iL<(*ssvec).size();++iL){
	if(iL<((*ssvec).size()-1)) {
	  unsigned next=iL+1;
	  absweight=(((*ssvec)[iL].voldEdx())+((*ssvec)[next].voldEdx()))/2. ;
	} else{
	  absweight+=(*ssvec)[iL].voldEdx()  ;
	}
	absW.push_back(absweight);
	absweight=0;
      }
      std::cout << " -- AbsWeight size: " << absW.size() << std::endl;
      std::cout<<" values are ";
      for (unsigned iL(0); iL<(*ssvec).size();++iL){
	std::cout<<" "<<absW[iL];
      }
      std::cout<<std::endl;
    }
    for (unsigned iL(0); iL<(*ssvec).size();++iL){
      h_ssvec->Fill(iL,absW[iL]);
    }

    //
    // information about generator level particles

    double ptgen=-1.;
    double Egen=-1.;
    double ptgenpx=-1.;
    double ptgenpy=-1.;
    double ptgenpz=-1.;
    double etagen=99999.;
    double phigen=99999.;
    double thetagen=-1.;
    int pidgen=-1;
    if((*genvec).size()>0) {
      pidgen=(*genvec)[0].pdgid();
      ptgenpx=(*genvec)[0].px()/1000.;
      ptgenpy=(*genvec)[0].py()/1000.;
      ptgenpz=(*genvec)[0].pz()/1000.;
      ptgen=sqrt(ptgenpx*ptgenpx+ptgenpy*ptgenpy);
      etagen=(*genvec)[0].eta();
      Egen=sqrt(ptgenpx*ptgenpx+ptgenpy*ptgenpy+ptgenpz*ptgenpz);
      phigen=(*genvec)[0].phi();
      thetagen=(*genvec)[0].theta();
    }
    h_getaphi->Fill(etagen,phigen);
    if(debug) {
      std::cout<<" gen vec size is "<<(*genvec).size()<<std::endl;
      std::cout<<" first gen pt E pid eta phi "<<ptgen<<" "<<Egen<<" "<<pidgen<<" "<<etagen<<" "<<phigen<<std::endl;
      for (unsigned iP(0); iP<(*genvec).size(); ++iP){
        std::cout<<" gen particle "<<iP<<" is "<<(*genvec)[iP].pdgid()<<std::endl;
      }
    }
    double etaaxis=etagen;
    double phiaxis=phigen;


    bool isScint = false;
    if (debug) std::cout << " - Event contains " << (*rechitvec).size() << " rechits." << std::endl;

    // make some simple plots about all the raw rechits, without the weights
    unsigned iMax=-1;
    double MaxE=-1.;
    bool snap=false;
    //double aistep=20000;
    double aistep=400;
    int istep=aistep;
    double stepsize=6000/aistep;
    for (unsigned iH(0); iH<(*rechitvec).size(); ++iH){//loop on hits
      HGCSSRecoHit lHit = (*rechitvec)[iH];
      double xh=lHit.get_x();
      double yh=lHit.get_y();
      double zh=lHit.get_z();
      double rh=sqrt(xh*xh+yh*yh);
      double Eh=lHit.energy();
      double leta = lHit.eta();
      double lphi = lHit.phi();
      unsigned layer = lHit.layer();
      unsigned ixx=layer;

      if(ixx>nscintlayer+scintoffset) ixx=ixx-nscintlayer-1;
      int ip=ixx-scintoffset;
      const HGCSSSubDetector & subdet = myDetector.subDetectorByLayer(layer);
      isScint = subdet.isScint;



      TH2Poly *map = isScint?(subdet.type==DetectorEnum::BHCAL1?geomConv.squareMap1():geomConv.squareMap2()): shape==4?geomConv.squareMap() : shape==2?geomConv.diamondMap() : shape==3? geomConv.triangleMap(): geomConv.hexagonMap();

      unsigned cellid = 0;
      ROOT::Math::XYZPoint pos = ROOT::Math::XYZPoint(lHit.get_x(),lHit.get_y(),lHit.get_z());
      if (isScint){
	double aaaphi = pos.phi();
	//if(aaaphi<0) aaaphi+=2.*TMath::Pi();
	cellid = map->FindBin(pos.eta(),aaaphi);
	//cellid = map->FindBin(pos.eta(),pos.phi());
      } else {
	cellid = map->FindBin(lHit.get_x(),lHit.get_y());
      }
      geomConv.fill(layer,Eh,0,cellid,zh);

      if(isScint) {
      if(firstEvent2) {
	firstEvent2=false;
	std::cout<<std::endl<<std::endl;
	std::cout<<"check check layer is "<<layer<<std::endl;
	double abceta=1.7;
	double abcphi;
	unsigned acellid=0;
	for(int i(0);i<100;i++) {
	  abcphi=(6.2/100.)*i;
	  acellid = map->FindBin(abceta,abcphi);
	  std::cout<<abceta<<" "<<abcphi<<" "<<acellid<<std::endl;
	}
	abceta=2.0;
	for(int i(0);i<100;i++) {
	  abcphi=(6.2/100.)*i;
	  acellid = map->FindBin(abceta,abcphi);
	  std::cout<<abceta<<" "<<abcphi<<" "<<acellid<<std::endl;
	}
      }
      }

      if(Eh>MaxE) {MaxE=Eh; iMax=iH;}
      if (debug>20) std::cout << " -- hit " << iH << " eta " << leta << std::endl; 

      h_cellid->Fill(cellid);

      
      h_cellidz->Fill(zh,cellid);

      h_energy->Fill(Eh);
      h_z->Fill(zh);
      h_z1->Fill(zh);
      h_z2->Fill(zh);
      h_l->Fill(layer+0.5);


      h_zl->Fill(zh,ixx);
      h_l2->Fill(layer+0.5);
      h_xy->Fill(xh,yh);
      h_etaphi->Fill(leta,lphi);


      if(ip>=0) { // look at layers with scintillator
	if(xh>5000) {  // weird hits
	  if(debug>5) std::cout<<"large x rechit "<<xh<<" y-hit z-hit layer energy cellis are "<<yh<<" "<<zh<<" "<<layer<<" "<<Eh<<" "<<cellid<<std::endl;
	  if(layer<badlaymin) badlaymin=layer;
	  if(!snap) {
	    if(isnap<nsnap-1) {
	      snap=true;
	      isnap+=1;
	      std::cout<<" isnap is "<<isnap<<std::endl;
	      SNAPrec_rl(h_snaprl[isnap],rechitvec);
	      SNAPrec_rz(h_snaprz[isnap],rechitvec);
	      SNAPsim(h_snaps[isnap],simhitvec,myDetector,geomConv,shape);
	    }
	  }
	  double rr=zh*tan(thetagen);
	  double xx=rr*cos(phigen);
	  double yy=rr*sin(phigen);
	  h_phibad->Fill(phigen);
	  h_xbad->Fill(xx);
	  h_ybad->Fill(yy);
	} else {  //normal his
	  
	  h_xrgood->Fill(rh,xh);
	  h_yrgood->Fill(rh,yh);
	}
	if(cellid<4000000000) {  // temp fix until Anne-Marie fixes cell geo problem
	  if(xh<xmin[ip]) {xmin[ip]=xh;}
          if(xh>xmax[ip]) {xmax[ip]=xh;}
	  if(yh<ymin[ip]) {ymin[ip]=yh;}
	  if(yh>ymax[ip]) {ymax[ip]=yh;}

	  if(isScint) {
	  //	  std::cout<<"ip rh is "<<ip<<" "<<rh<<std::endl;
	  //std::cout<<rmins[ip]<<" "<<rmaxs[ip]<<std::endl;
	    h_sxy[ip]->Fill(xh,yh);

	    h_cellidzs->Fill(zh,cellid);
	    h_cellidphis->Fill(pos.phi(),cellid);
	    h_cellidetas->Fill(pos.eta(),cellid);
	    if(domap) {
	    if(ipfirst[ip]) {
	      ipfirst[ip]=false;
	      std::cout<<"doing ip "<<ip<<std::endl;
	      for(int iia(0);iia<istep;iia++) {
		for(int iib(0);iib<istep;iib++) {
		  //double xxa=-3000.+stepsize*iia;
		  //double yya=-3000.+stepsize*iib;
		  double etaa=1.2+((4.-1.2)/aistep)*iia;
		  double phia=-3.14+((6.4)/aistep)*iib;
		  double thetaa=2.*atan(exp(-etaa));
		  double ra=lHit.get_z()*tan(thetaa);
		  double xxa=ra*cos(phia);
		  double yya=ra*sin(phia);
		  ROOT::Math::XYZPoint apos = ROOT::Math::XYZPoint(xxa,yya,lHit.get_z());
		  //double bbbphi = phia;
		  double bbbphi=apos.phi();
		  //if(bbbphi<0) bbbphi+=2.*TMath::Pi();
		  //unsigned acellid = map->FindBin(etaa,bbbphi);
		  unsigned acellid = map->FindBin(apos.eta(),bbbphi);
		  //unsigned acellid = map->FindBin(apos.eta(),apos.phi());
		  if(acellid<4000000000) {
		    if((apos.x()<-500)&&(apos.y()<-500) ) {
		    if((apos.x()>-600)&&(apos.y()>-600) ) {
		      //		      std::cout<<"michael "<<ip<<" "<<apos.x()<<" "<<apos.y()<<" "<<apos.eta()<<" "<<apos.phi()<<" "<<acellid<<std::endl;
		    }
		    }
		    if((apos.x()>500)&&(apos.y()>500) ) {
		    if((apos.x()<600)&&(apos.y()<600) ) {
		      //		      std::cout<<"michael "<<ip<<" "<<apos.x()<<" "<<apos.y()<<" "<<apos.eta()<<" "<<apos.phi()<<" "<<acellid<<std::endl;
		    }
		    }

		    double abc=h_scellid[ip]->GetBinContent(apos.phi(),apos.eta());
		    if(abc<0.5) h_scellid[ip]->Fill(apos.phi(),apos.eta(),acellid);
		    abc=h_scellidxy[ip]->GetBinContent(apos.x(),apos.y());
		    if(abc<0.5) h_scellidxy[ip]->Fill(apos.x(),apos.y(),acellid);
		    abc=h_scellidxyzoom[ip]->GetBinContent(apos.x(),apos.y());
		    if(abc<0.5) h_scellidxyzoom[ip]->Fill(apos.x(),apos.y(),acellid);
		    h_scellideta[ip]->Fill(apos.eta(),acellid);
		    h_scellidphi[ip]->Fill(apos.phi(),acellid);

	if(acellid>ibinScintmax[ip]) ibinScintmax[ip]=acellid;
	if(acellid<ibinScintmin[ip]) ibinScintmin[ip]=acellid;

		  }
		}
	      }
	    }
	    }

	    if(rh<rmins[ip]) {rmins[ip]=rh;}
	    if(rh>rmaxs[ip]) {rmaxs[ip]=rh;}
	  //std::cout<<rmins[ip]<<" "<<rmaxs[ip]<<std::endl;

	  } else {  // end if scint
	    h_nsxy[ip]->Fill(xh,yh);
	    if(rh<rminns[ip]) {rminns[ip]=rh;}
	    if(rh>rmaxns[ip]) {rmaxns[ip]=rh;}
	  }
	}  // end temp fix
      }  // end loooking at layers with scipt (ip>0)
    }  // end loop over hits



    HGCSSRecoHit lHit = (*rechitvec)[iMax];
    double maxeta = lHit.eta();
    double maxphi=lHit.phi();
    double maxE=lHit.energy();
    h_maxE->Fill(maxE);
    h_etagenmax->Fill(maxeta,etagen);
    h_phigenmax->Fill(maxphi,phigen);
    h_phimax->Fill(maxphi);
    h_etamax->Fill(maxeta);
    if(debug>2) {
      std::cout<<" Max hit energy eta phi "<<lHit.energy()<<" "<<lHit.eta()<<" "<<lHit.phi()<<std::endl;
    }



    // make e/p plots for various cones around gen particle, using weights this time
    // for now, scale up by 10.  don't know why.  asking Anne-Marie
    // also change to GeV for this section
    const unsigned isize=5;
    double coneSize[isize]={0.1,0.2,0.3,0.4,0.5};
    double rechitsum[isize]={0.,0.,0.,0.,0.};
    double rechitsumdead[isize]={0.,0.,0.,0.,0.};
    double rechitBHsum[isize]={0.,0.,0.,0.};

    double etaW=0.;
    double phiW=0.;
    double norm=0.;
    for (unsigned iH(0); iH<(*rechitvec).size(); ++iH){//loop on hits

      HGCSSRecoHit lHit = (*rechitvec)[iH];
      unsigned layer = lHit.layer();

      int ip=layer-scintoffset-nscintlayer-1;

      const HGCSSSubDetector & subdet = myDetector.subDetectorByLayer(layer);
      isScint = subdet.isScint;
      double leta = lHit.eta();
      double lphi = lHit.phi();
      double lenergy=lHit.energy()*absW[layer]/1000.;
      TH2Poly *map = isScint?(subdet.type==DetectorEnum::BHCAL1?geomConv.squareMap1():geomConv.squareMap2()): shape==4?geomConv.squareMap() : shape==2?geomConv.diamondMap() : shape==3? geomConv.triangleMap(): geomConv.hexagonMap();
      unsigned cellid = 0;
      ROOT::Math::XYZPoint pos = ROOT::Math::XYZPoint(lHit.get_x(),lHit.get_y(),lHit.get_z());
      if (isScint){
	double aaaphi = pos.phi();
	//if(aaaphi<0) aaaphi+=2.*TMath::Pi();
	cellid = map->FindBin(pos.eta(),aaaphi);
	//cellid = map->FindBin(pos.eta(),pos.phi());
      } else {
	cellid = map->FindBin(lHit.get_x(),lHit.get_y());
      }

      if (debug>20) std::cout << " -- hit " << iH << " et eta phi " << lenergy<<" "<<leta << " "<< lphi<<std::endl; 
	//clean up rechit collection

      norm+=lenergy;
      etaW+=leta*lenergy;
      phiW+=lphi*lenergy;

      double dR=DeltaR(etaaxis,phiaxis,leta,lphi);
      //double dR=fabs(etagen-leta);
      if(debug>20) std::cout<<" dR "<<dR<<" "<<etagen<<" "<<phigen<<" "<<leta<<" "<<lphi<<std::endl;
      for(unsigned ii(0);ii<isize;ii++) {
	if(dR<coneSize[ii]) {
	  rechitsum[ii]+=lenergy;
	 
	  if(isScint) {
	    rechitBHsum[ii]+=lenergy;
	    std::pair<unsigned,unsigned> temp(ip,cellid);
	    std::set<std::pair<unsigned,unsigned>>::iterator ibc=deadlist.find(temp);
	    //std::cout<<"michael "<<temp.first<<" "<<temp.second<<std::endl;
	    //std::cout<<" I am here"<<std::endl;
	    //if(ibc!=deadlist.end()) std::cout<<" found on dead list"<<std::endl;
	    if(ibc==deadlist.end()) {
	      rechitsumdead[ii]+=lenergy;
	    }
	    
	  } else {
	    rechitsumdead[ii]+=lenergy;
	  }
	  
	}
      }


    }//loop on hits
    if(debug>1) {
      std::cout<<" reco gen are "<<rechitsum[0]<<" "<<rechitsum[1]<<" "<<rechitsum[2]<<" "<<rechitsum[3]<<" "<<rechitsum[4]<<" "<<Egen<<std::endl;
    }
    if(debug>5) std::cout<<"weighted eta phi are "<<etaW/norm<<" "<<phiW/norm<<std::endl;

    h_Egenreco->Fill(Egen,rechitsum[3]/Egen);
    h_egenreco->Fill(rechitsum[3]/Egen);
    if(phigen>0) {
      h_egenrecopphi->Fill(rechitsum[3]/Egen);
      h_egenrecopphidead->Fill(rechitsumdead[3]/Egen);
    }
    else {h_egenrecomphi->Fill(rechitsum[3]/Egen);}
    h_EpPhi->Fill(phigen,rechitsum[3]/Egen);
    for(unsigned ii(0);ii<isize;ii++) {
      h_EpCone->Fill(coneSize[ii],rechitsum[ii]/Egen);
    }
    h_banana->Fill(rechitsum[3]-rechitBHsum[3],rechitBHsum[3]);
    double frac=-0.05;
    double notBH=rechitsum[3]-rechitBHsum[3];
    if(rechitsum[3]>0) frac=rechitBHsum[3]/rechitsum[3];
    h_fracBH->Fill(frac);
    

    h_ECone03->Fill(rechitsum[3]);





    // make plots about simhits

    double simHitSum=0.;
    double maxSim=0.;
    double msxH=0.;
    double msyH=0.;
    double mszH=0.;
    double msleta=0.;
    double mslphi=0.;
    unsigned mscellid=0;

    unsigned isimMax=-1;
    if(debug>2) std::cout<<"stuff on sim hits"<<std::endl;
    for (unsigned iH(0); iH<(*simhitvec).size(); ++iH){//loop on hits
      HGCSSSimHit lHit = (*simhitvec)[iH];
      unsigned layer = lHit.layer();
      const HGCSSSubDetector & subdet = myDetector.subDetectorByLayer(layer);
      ROOT::Math::XYZPoint haha = lHit.position(subdet,geomConv,shape);
      isScint = subdet.isScint;
      unsigned lcellid=lHit.cellid();
      double xH=haha.x();
      double yH=haha.y();
      double zH = haha.z();
      double rH=sqrt(xH*xH+yH*yH);
      double tant = rH/zH;
      double theta=atan(tant);
      double leta=-log(tan(theta/2.));
      double lphi = atan2(yH,xH);
      double lenergy=lHit.energy()*mycalib.MeVToMip(layer,rH)*absW[layer]/1000.;
      double time = lHit.time();
      unsigned ilay = lHit.silayer();


      h_simD1->Fill(leta,lphi,lHit.energy());
      h_simD2->Fill(leta,lphi,mycalib.MeVToMip(layer,rH));
      h_simD3->Fill(mycalib.MeVToMip(layer,rH));
      h_simD4->Fill(layer,mycalib.MeVToMip(layer,rH));
      h_simD5->Fill(rH,mycalib.MeVToMip(layer,rH));
		    

      if(lHit.energy()>0) std::cout<<xH<<" "<<yH<<" "<<zH<<" "<<layer<<" "<<rH<<" "<<leta<<" "<<lphi<<" "<<lHit.energy()<<" "<<mycalib.MeVToMip(layer,rH)<<" "<<lenergy<<" "<<time<<" "<<ilay<<std::endl;

      if(lcellid>4000000000) {
	std::cout<<" weird rechit cellid x y z E "<<lcellid<<" "<<xH<<" "<<yH<<" "<<zH<<" "<<lenergy<<std::endl;
	h_eWeirdSim->Fill(lenergy);
      }
      if(lenergy>maxSim) {
	maxSim=lenergy;
	msxH=xH;
	msyH=yH;
	mszH=zH;
	msleta=leta;
	mslphi=lphi;
	isimMax=iH;
	mscellid=lcellid;
      }

      double dR=DeltaR(etaaxis,phiaxis,leta,lphi);
      if(dR<0.3) {
	simHitSum+=lenergy;
      }

    }//loop on simhits

    h_simHit03->Fill(simHitSum);
    h_simD6->Fill(rechitsum[3],simHitSum);

    double logmaxcellid=log10(4293967295);
    if(mscellid>0) logmaxcellid=log10(mscellid);
    h_cellids->Fill(logmaxcellid);
    double dR=DeltaR(etaaxis,phiaxis,msleta,mslphi);
    if(dR>0.5) {
      h_badcellids->Fill(logmaxcellid);
    }
    /*
    // add alusumhits
    for (unsigned iH(0); iH<(*alusimhitvec).size(); ++iH){//loop on hits
      HGCSSSimHit lHit = (*simhitvec)[iH];
      unsigned layer = lHit.layer();
      const HGCSSSubDetector & subdet = myDetector.subDetectorByLayer(layer);
      ROOT::Math::XYZPoint haha = lHit.position(subdet,geomConv,shape);
      double xH=haha.x();
      double yH=haha.y();
      double zH = haha.z();
      double rH=sqrt(xH*xH+yH*yH);
      double tant = rH/zH;
      double theta=atan(tant);
      double leta=-log(tan(theta/2.));
      double lphi = atan2(yH,xH);
      //double lenergy=lHit.energy()*absW[layer]/1000.;
      double lenergy=lHit.energy()/1000.;
      //double lenergy=lHit.energy()*mycalib.MeVToMip(layer,rH)*absW[layer]/1000.;

      double dR=DeltaR(etaaxis,phiaxis,leta,lphi);
      if(dR<0.3) {
	simHitSum+=lenergy;
      }

    }//loop on simhits
    */

    if(debug>2) {
      std::cout<<"max sim hit cellid x y z eta phi energy "<<mscellid<<" "<<msxH<<" "<<msyH<<" "<<mszH<<" "<<msleta<<" "<<mslphi<<" "<<maxSim<<std::endl;
      std::cout<<"sim hit sum is "<<simHitSum<<std::endl;
    }





      //miptree->Fill();

    geomConv.initialiseHistos();
    ievtRec++;
  }//loop on entries

    if(debug) {
      std::cout<<std::endl<<std::endl;
      for(int i(0);i<nscintlayer;i++) {
	std::cout<<" min max scint cellid layer "<<i<<"  are "<<ibinScintmin[i]<<" "<<ibinScintmax[i]<<" "<<std::endl;
      }
      std::cout<<std::endl<<std::endl<<"rmin max for layer"<<std::endl;;
      for(unsigned ii(0);ii<nscintlayer;ii++){
	std::cout<<"   "<<ii+scintoffset<<" "<<rminns[ii]<<" "<<rmaxns[ii]<<" "<<rmins[ii]<<" "<<rmaxs[ii]<<std::endl;
      }
      std::cout<<std::endl<<std::endl<<"xmin max for layer"<<std::endl;;
      for(unsigned ii(0);ii<nscintlayer;ii++){
	std::cout<<"   "<<ii+scintoffset<<" "<<xmin[ii]<<" "<<xmax[ii]<<std::endl;
      }
      std::cout<<std::endl<<std::endl<<"ymin max for layer"<<std::endl;;
      for(unsigned ii(0);ii<nscintlayer;ii++){
	std::cout<<"   "<<ii+scintoffset<<" "<<ymin[ii]<<" "<<ymax[ii]<<std::endl;
      }
    }

    if(debug){ std::cout<<"badlayermin is "<<badlaymin<<std::endl;}

  if(debug) std::cout<<"writing files"<<std::endl;

  outputFile->cd();
  outputFile->Write();
  outputFile->Close();

  return 0;
  
}//main
