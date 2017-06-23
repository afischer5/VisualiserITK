#include "ShowPixelEndcap.h"
#include <cmath>

// implementation of the endcap drawing
bool ShowPixelEndcap::process(InDet::XMLReaderSvc& reader, TGeoVolume* top, TGeoVolume* innerDetector, TGeoVolume* fullDetector, TGeoManager* geom,int complexity)
{

   // define some constants
   const double degree=57.2958;
   const double PI=TMath::Pi();
   const double PI2=2*TMath::Pi();

//get materials from xml reader
   bool hasModuleMat = false;
   bool hasRingMat = false;       //booleans to check if the materials were gathered
   std::vector<MaterialTmp *> materials = reader.getMaterials();
   TGeoMaterial *matModule;
   TGeoMedium *medModule;
   TGeoMaterial *matRing;
   TGeoMedium *medRing; 
   for(int m = 0; m<materials.size();m++){
     MaterialTmp* material = materials.at(m);
     if(material->name=="DefaultPixelModuleMaterial"){
        matModule = new TGeoMaterial("PixelModuleMaterial",0,0,material->density);
        medModule = new TGeoMedium("PixelModuleMedium",1,matModule);
        hasModuleMat = true;
     }
     if(material->name=="DefaultPixelRingMaterial"){
        matRing = new TGeoMaterial("PixelRingMaterial",0,0,material->density);
        medRing = new TGeoMedium("PixelRingMedium",2,matRing);
	hasRingMat = true;
     }
   }
   TGeoMaterial *matVacuum = new TGeoMaterial("Vacuum", 0,0,0);  //default materials to use in case parser cannot gather specific materials
   TGeoMaterial *matAl = new TGeoMaterial("Al", 26.98,13,2.7);
   TGeoMedium *Vacuum = new TGeoMedium("Vacuum",1, matVacuum);
   TGeoMedium *Al = new TGeoMedium("Aluminium",2, matAl);
   TGeoMedium *Si = new TGeoMedium("Si",7,7,0,0,0,20,0.1000000E+11,0.212,0.1000000E-02,1.150551);
	
        //Create assemlblies for inner and outer
	string innerPixelEndcapsName = "InnerPixelEndcaps";
	TGeoVolume *innerPixelEndcaps = new TGeoVolumeAssembly(innerPixelEndcapsName.c_str());
	string PixelEndcapsName = "PixelEndcaps";
        TGeoVolume *PixelEndcaps = new TGeoVolumeAssembly(PixelEndcapsName.c_str());
	

//Build Layers for discs
	std::vector< EndcapLayerTmp *> layers = reader.getPixelEndcapLayers();

	for(unsigned int i=0; i<layers.size();i++) {
		EndcapLayerTmp *layer = layers.at(i);
     		string layername="EndcapLayerAssembly"; 
      		TGeoVolume *assembly_layer = new TGeoVolumeAssembly(layername.c_str());
		
		layer->Print();	

//Build rings in layer
			
		int nRings = layer->ringpos.size();
			
 		for(int iR=0;iR<nRings;++iR){

			string ringname = "EndcapRingAssembly";
               		TGeoVolume *assembly_rings = new TGeoVolumeAssembly(ringname.c_str());

			double ringposition = layer->ringpos[iR];
			double innerradius;
			double outerradius;
			string modType = layer->modtype[0];
			double totalRingThickness = layer->thickness[0];
			
			if(layer->innerRadius[1]==0){      //determines if using vectors or single values
				innerradius=layer->innerRadius[0];
			}
			else{
				innerradius=layer->innerRadius[iR];
			}
			
			if(layer->outerRadius[1]==0){
                                outerradius=layer->outerRadius[0];
                        }
                        else{
                                outerradius=layer->outerRadius[iR];
                        }
			if(layer->modtype[1]==0){
				modType = layer->modtype[0];
			}
			else{
				modType = layer->modtype[iR];
			}
			if(layer->thickness[1]!=0)
				totalRingThickness=layer->thickness[iR];

			string halfringname = "EndcapHalfRingSupport";
			TGeoVolume *ring_obj = geom->MakeTubs(halfringname.c_str(),medRing,innerradius,outerradius,layer->zoffset[0],-1*PI*degree/2,PI*degree/2);
			ring_obj->SetLineColor(kBlack);
			ring_obj->SetTransparency(65);	
						
			TGeoRotation *rot = new TGeoRotation();
			rot->RotateZ(180);		//sets up rotation for other half of endcap 
			
					
			//-------------Sectors added here
			if(complexity!=2){
			int nSectors;
			double angleSector;
			if(layer->nsectors[1]==0){
				nSectors = layer->nsectors[0];
				angleSector =(PI2*degree)/nSectors;

			}
			else{
				nSectors = layer->nsectors[iR];
				angleSector =(PI2*degree)/nSectors;	//the angle of the arc taken up by each sector
			}

			//Build Sectors
			for(int iS = 0; iS<nSectors/2;++iS){
				TGeoVolume *sect_obj = geom->MakeTubs(modType.c_str(),medModule,innerradius,outerradius,totalRingThickness-layer->zoffset[0],(-1*PI*degree)/2+iS*angleSector,(-1*PI*degree)/2+iS*angleSector+angleSector);
				sect_obj->SetLineColor(4);
				sect_obj->SetTransparency(60);
				//if(layer->double_sided)	//needs to be tested with an xml file that has double sided endcaps
					//sect_obj =sect_obj->Divide(modType,3,2,1,1);
				ring_obj->AddNode(sect_obj,iS+1,new TGeoTranslation(0,0,layer->zoffset[0]));
				ring_obj->AddNode(sect_obj,iS+1,new TGeoTranslation(0,0,-layer->zoffset[0]));
			}
			}
			
			

			//------adds rings to assemblies
			assembly_rings->AddNode(ring_obj,iR+1,new TGeoCombiTrans(0,0,ringposition+(layer->splitOffSet/2),0));
			assembly_rings->AddNode(ring_obj,iR+1,new TGeoCombiTrans(0,0,ringposition-(layer->splitOffSet/2),rot));//halfrings are shifted away to make room for services
			if(complexity!=1){
                        assembly_rings->AddNode(ring_obj,iR+1+nRings,new TGeoCombiTrans(0,0,-ringposition+(layer->splitOffSet/2),0));//not sure which side is shifted in which direction right now its arbitrary
                        assembly_rings->AddNode(ring_obj,iR+1+nRings,new TGeoCombiTrans(0,0,-ringposition-(layer->splitOffSet/2),rot));
                       	//^^^^adds the other side of the detector as a mirror provided the user set complexity at an appropriate value
                        }

			assembly_layer->AddNode(assembly_rings,iR+1,new TGeoTranslation(0,0,0));
			
		}
		//puts the layers in the correct inner or full assembly
		if(i<2) innerPixelEndcaps->AddNode(assembly_layer, i+1, new TGeoTranslation(0,0,0));
		PixelEndcaps->AddNode(assembly_layer, i+1, new TGeoTranslation(0,0,0));
			
	}

   innerDetector->AddNode(innerPixelEndcaps,1,new TGeoTranslation(0,0,0));
   fullDetector->AddNode(PixelEndcaps,1,new TGeoTranslation(0,0,0));
   return true;
} 


