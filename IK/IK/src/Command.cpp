#ifndef __COMMAND_H__
#include "Command.h"
#endif //__COMMAND_H__
 
#ifndef __C3DFILEINFO_H__
#include "C3dFileInfo.h"
#endif  //__C3DFILEINFO_H__
 
#ifndef __ARTICULATEDBODY_H__
#include "ArticulatedBody.h"
#endif	//__ARTICULATEDBODY_H__
 
#ifndef RealTimeIKui_h
#include "RealTimeIKui.h"
#endif //RealTimeIKui_h
 
#ifndef __PHYLTERFLGLWINDOW_H__
#include "PhylterGLWindow.h"
#endif	//__PHYLTERFLGLWINDOW_H__
 
#ifndef	__PHOWARDDATA_H__
#include "PhowardData.h"
#endif
 
#ifndef __TRANSFORM_H__
#include "Transform.h"
#endif	//__TRANSFORM_H__
 
 
 
int readSkelFile( FILE* file, ArticulatedBody* skel );
 
extern RealTimeIKUI *UI;
bool solve = false;

Vec3d c=Vec3d();
Vecd w = Vecd();
TMat Jacobian = TMat();
extern vector<Vec3d> handles;
vector<Vec3d> cVals= vector<Vec3d>();


double alpha = 0.02;

void LoadModel(void *v)
{
  char *params = (char*)v;
  if(!params){
    params = (char*)fl_file_chooser("Open File?", "{*.skel}", "../src/skels" );
  }
 
  if(!params)
    return;
  FILE *file = fopen(params, "r");
    
  if(file == NULL){
    cout << "Skel file does not exist" << endl;
    return;
  }
 
  ArticulatedBody *mod = new ArticulatedBody();
  UI->mData->mModels.push_back(mod);
  UI->mData->mSelectedModel = mod;
 
  readSkelFile(file, mod);
  UI->CreateDofSliderWindow();
 
  mod->InitModel();
  UI->mGLWindow->mShowModel = true;
  UI->mShowModel_but->value(1);
  UI->mGLWindow->refresh();
  
  cout << "number of dofs in model: " << UI->mData->mModels[0]->GetDofCount() << endl;
}
 
void Solution(void *v)
{
	Jacobian.SetSize(UI->mData->mSelectedModel->GetHandleCount() * 3,UI->mData->mSelectedModel->GetDofCount());
	Jacobian.MakeZero();
	//cout << "Initial Jacobian " << Jacobian << "\n";
	int frame = 0;
	solve=true;
	
	w = CalculateWeights(frame);
	//cout << "FQ " << CalculateFQ(frame) << "\n";
	if(CalculateFQ(frame) > 0.001){
		//loop over the handles
		Vecd pFpq = Vecd(UI->mData->mSelectedModel->GetDofCount());
		pFpq.MakeZero();
		
		//cout << "Weights " << w << "\n";


		for(int handle = 0; handle < UI->mData->mSelectedModel->GetHandleCount(); handle++){
			//cout << "Operating on handle " << handle << "\n";
			//reset jacobian so we dont add several iterations worth of data
			Jacobian.MakeZero();

			//compute the jacobian for handle i
			ComputeJ(handle); //Ji is now in Jacobian
			//cout << "Jacobian computed \n";

			//compute the transpose for handle i
			TMat Jti = trans(Jacobian); // Jti is transpose of jacobian at entry i
			//cout << "Jacobian transposed ";

			//dFdQ = dFdq + (wi * (Jti*Ci))
			//cout << "Jacobian " << Jti << "\n";
			//cout << "C " << CalculateC(handle) << "\n";
			//pFpq += (w[handle]) * (Jti * CalculateCVec(handle,frame));
			//cout << " C vector " << CalculateCVec(handle,frame) << "\n";
			pFpq += (Jti * CalculateCVec(handle,frame));
			//cout << "Jacobian summed ";
		}
		pFpq *= 2;
		Vecd qNew = Vecd(pFpq.Elts());
		//cout << "Q: ";
		for(int i=0; i<pFpq.Elts(); i++){
			double qOld = UI->mData->mSelectedModel->mDofList.GetDof(i);
			//cout << qOld << ", ";
			qNew[i] = qOld - alpha * pFpq[i];
		}
		//cout << "\n";
		//cout << " Q new " << qNew << "\n";
		UI->mData->mSelectedModel->SetDofs(qNew);
		Fl::add_timeout(0.001,Solution);
	}
	//cout << "Finished solving \n";
}
void Exit(void *v)
{
  exit(0);
}
 
void LoadC3d(void *v)
{
  if(!UI->mData->mSelectedModel){
    cout << "Load skeleton first";
    return;
  }
  char *params = (char*)v;
  if(!params){
    params = fl_file_chooser("Open File?", "{*.c3d}", "mocap/" );
  }
 
  if(!params)
    return;
  
  char *c3dFilename = new char[80];
  
  // load single c3d file
 
  C3dFileInfo *openFile = new C3dFileInfo(params);
  openFile->LoadFile();
  UI->mData->mSelectedModel->mOpenedC3dFile = openFile;
  cout << "number of frames in c3d: " << openFile->GetFrameCount() << endl;
 
  UI->InitControlPanel();
  UI->mGLWindow->mShowConstraints = true;
  UI->mShowConstr_but->value(1);
}
Vec3d CalculateC(int handle, int frame){
	 Marker* mark = UI->mData->mSelectedModel->mHandleList[handle];
	 
	 Vec3d pBar = UI->mData->mSelectedModel->mOpenedC3dFile->GetMarkerPos(frame,handle);
	 //cout << "pBar is " << pBar << "\n";
	 if(pBar == Vec3d(0,0,0))
		 return Vec3d(0,0,0);
	 return mark->mGlobalPos-pBar;
	 //cVals.push_back(mark->mGlobalPos-pBar);
}

 Vecd CalculateCVec(int handle, int frame){
	 Vecd ret = Vecd(UI->mData->mSelectedModel->GetHandleCount() * 3);
	 ret.MakeZero();
	 Vec3d cvals = CalculateC(handle,frame);
	 for(int i=0; i<3; i++){
		 ret[3*handle + i] = cvals[i]; 
	 }
	 return ret;

 }
void CalculateC(){
	Marker* mark=UI->mData->mSelectedModel->mHandleList[0];
	Vec3d pBar=UI->mData->mSelectedModel-> mOpenedC3dFile->GetMarkerPos(0,0);
	Vec3d handlePos=mark->mGlobalPos;
	c=mark->mGlobalPos-pBar;
}
void computeJ(){
	Marker* mark=UI->mData->mSelectedModel->mHandleList[0];
	//node we're computing partials for
	TransformNode* node = UI->mData->mSelectedModel->mLimbs[mark->mNodeIndex];
	//remaining transformations
	int NeedOffset = 1;
	Vec4d Ji;
	Vec4d u = Vec4d(1,1,1,1);
	Mat4d um = Mat4d(vl_1);
	/** While there are still nodes to process **/
	while(node != NULL){
		Mat4d parent = node->mParentTransform;
		//loop over the transforms for this node
		cout << "Operating on node " << node->mName << "\n";
 
		for(int trans=0; trans<node->mTransforms.size(); trans++){
			Transform* current = node->mTransforms[trans];
			
 
			//determine if the current transform is a dof
			if(current->IsDof()){
				//loop over the DOF's in the transform
 
 
				for(int dof=0; dof<current->GetDofCount(); dof++){
					//compute partial derivative
					Mat4d partial = current->GetDeriv(dof);
					//cout << "Deriv is " << partial << "\n";
					Mat4d Jim = parent;

 
 
					//compute jacobian entry & u (as matrices)
					for(int i=0; i<node->mTransforms.size(); i++){
						if(i == trans){
							Jim *= partial;
						}else{
							Jim *= node->mTransforms[i]->GetTransform();
						}
						um *= node->mTransforms[i]->GetTransform();
					}
 
					
					//multiply by offset -- only if at the foot joint
					if(NeedOffset == 1){
						Ji = Jim * Vec4d(mark->mOffset,1);
						u = um * Vec4d(mark->mOffset,1);
					}
					else{
						u = u * um;
						Ji = Jim * u;
					}
 
 
					//compute row & column of entry
					int column = current->GetDof(dof)->mId;
 
					//set jacobian column
					for(int j=0; j<3; j++){\
						//Jacobian[j][column] = Ji[j];
						Jacobian[j][column] = Ji[j];
					}
 

					
				}//end DOF loop
 
 
 
			}//end dof check
 
 
		}//end transform loop
		NeedOffset = 0;
		node = node->mParentNode;
	}//end while
}

void ComputeJ(int handle){

	Marker* mark=UI->mData->mSelectedModel->mHandleList[handle];
	TransformNode* node = UI->mData->mSelectedModel->mLimbs[mark->mNodeIndex];


	int NeedOffset = 1;
	Vec4d u = Vec4d(mark->mOffset,1);

	while(node != NULL){
		Mat4d parent = node->mParentTransform;

		for(int trans=0; trans<node->mTransforms.size(); trans++){
			Transform* current = node->mTransforms[trans];

			if(current->IsDof()){
				for(int dof = 0; dof < current->GetDofCount(); dof++){
					Mat4d Ji = parent;
					for(int i=0; i<node->mTransforms.size(); i++){
						if(i == trans){
							Ji *= current->GetDeriv(dof);
						}
						else
							Ji *= node->mTransforms[i]->GetTransform();
					}//end inner transform

					Vec4d Jentry = Ji * u;
					int column = current->GetDof(dof)->mId;
					for(int j=0; j<3; j++){
						Jacobian[(3*handle) + j][column] = Jentry[j];
					}//end assigning jacobian row
				}//end dof loop
			}//end dof check
		}//end outer transform loop

		u *= node->mCurrentTransform;
		node = node->mParentNode;
	}
}
double CalculateFQ(int frame){
	double total = 0;
	for(int handle = 0; handle < UI->mData->mSelectedModel->GetHandleCount(); handle++){
		//cout << "Calculating FQ for handle " << handle << "\n";
		//cout << "C for handle is " << CalculateC(handle,frame) << "\n";
		double length = len(CalculateC(handle,frame));
		double error = length * length;
		error *= w[handle];
		total += error;
	}
	//cout << "Total error is " << total << "\n";
	return total;
}

double CalculateFQ(int handle, int frame){
	double length = len(CalculateC(handle,frame));
	double error = length * length;
	return error;
}

bool KeepGoing(){
	//loop over handles, if there exists one that needs to be moved we keep going
	for(int handle = 0; handle < UI->mData->mSelectedModel->GetHandleCount(); handle++){
		if(CalculateFQ(handle) > 0.0025)
			return true;
	}
	return false;
}

Vecd CalculateWeights(int frame){
	Vecd ret = Vecd(UI->mData->mSelectedModel->GetHandleCount());
	for(int i=0; i<UI->mData->mSelectedModel->GetHandleCount(); i++){
		//cout << "Length of c " << len(CalculateC(i,frame)) << "\n";

		ret[i] = 1;
	}
	return ret;
}