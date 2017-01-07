#include "CNN.h"
#include "vector"
#include "iostream"
#include "match_util.h"
using namespace std;
#define DSIGMOID(S) (S*(1-S)) // derivative of the sigmoid as a function of the sigmoid's output
void CNN::train(double ***train_x,double** train_label,int NumOfImage)
{
	cout<<"Start train"<<endl;
	int epochsNum =NumOfImage/batchSize;
	if(NumOfImage%batchSize != 0)
	{
		cout<<"Please reset batchSize!"<<endl;
	}
	double ***Train;
	double** TrainLabel;
	int* randPerm ;
	Train = new double **[batchSize];
	TrainLabel = new double *[batchSize];
	randPerm = new int[NumOfImage];
	randperm(NumOfImage, randPerm);
	
	for ( int i=0; i<epochsNum;i++)
	{
		for ( int j=0; j<batchSize; j++)
		{
			//Train[j] = train_x[randPerm[i*batchSize+j]];
			//TrainLabel[j] = train_label[randPerm[i*batchSize+j]];
			if (i == 0)
			{
				Train[j] = new double *[Len];
			}
			for (int m=0; m<Len; m++)
			{
				if (i ==0)
				{
					Train[j][m] = new double [Len];
				}				
				for (int n=0; n<Len; n++)
				{
					Train[j][m][n] = train_x[randPerm[i*batchSize+j]][m][n];
				}
			}
			if (i == 0)
			{
				TrainLabel[j] = new double [10];
			}
			for (int l=0;l<10;l++)
			{
				TrainLabel[j][l] = train_label[randPerm[i*batchSize+j]][l];
			}
		}
		forward(Train);
		backPropagation(Train,TrainLabel);
		updateParas();
		//cout<<"Finish "<<i+1<<" train"<<endl;
	}	
    cout<< "Finish train"<<endl;
	delete []randPerm;
	for (int i=0; i<batchSize; i++)
	{
		delete []TrainLabel[i];
		for (int j=0; j<Len; j++)
		{
			delete []Train[i][j];
		}
		delete []Train[i];
	}
	delete []Train;
	delete []TrainLabel;
}

void CNN::setup(int batchSize)
{
	layers::iterator iter = m_layers.begin();
	//Layer inputLayer = this->m_layers[0];
	(*iter).initOutputmaps(batchSize);
	iter++;
	for ( iter ;iter < m_layers.end();iter++)
	{
		
		//Layer layer=this->m_layers[i];
		//Layer frontLayer = this->m_layers[i-1];
		//int frontMapNum = frontLayer.getOutMapNum();
		int frontMapNum = (*(iter-1)).getOutMapNum();
		
		switch ((*iter).getType())
		{
		case 'I':
			break;
		case 'C':
			// set map size
			(*iter).setMapSize((*(iter-1)).getMapSize().substract((*iter).getKernelSize(),1));
			// initial convolution kernel, quantities: frontMapNum*outMapNum
			(*iter).initKernel(frontMapNum);
			//each map has one bias, so frontMapNum is not necessary
			(*iter).initBias(frontMapNum);
 			(*iter).initErros(batchSize);
 			// each layer should initialize output map
 			(*iter).initOutputmaps(batchSize);
			break;
		case 'S':
			(*iter).setOutMapNum((frontMapNum));
			(*iter).setMapSize((*(iter-1)).getMapSize().divide((*iter).getScaleSize()));
			(*iter).initErros(batchSize);
			(*iter).initOutputmaps(batchSize);
			break;
		case 'O':
			(*iter).initOutputKernel(frontMapNum, (*(iter-1)).getMapSize());
			(*iter).initBias(frontMapNum);
			(*iter).initErros(batchSize);
			(*iter).initOutputmaps(batchSize);
			break;
		default:
			break;
		}
	}
}


void CNN::backPropagation(double*** x,double** y)
{
	setOutLayerErrors(x,y);
	setHiddenLayerErrors();
}

void CNN::forward(double ***x)
{
	setInLayerOutput(x);
	layers::iterator iter = m_layers.begin();
	iter++;
	for(iter; iter<m_layers.end(); iter++)
	{
		switch((*iter).getType())
		{
		case 'C':
			setConvOutput((*iter), (*(iter-1)));
			break;
		case 'S':
			setSampOutput((*iter), (*(iter-1)));
			break;
		case 'O':
			setConvOutput((*iter), (*(iter-1)));
			break;
		default:
			break;
		}
		
	}
}

void CNN::setInLayerOutput(double*** x)
{
	layers:: iterator iter = m_layers.begin();
	//Layer inputLayer = m_layers[0];
	size mapSize = (*iter).getMapSize();
 	if(Len != mapSize.x)
	{
 		cout<<"���ݼ�¼�Ĵ�С�붨���map��С��һ��!" << endl;	
	}
	for(int i=0;i<batchSize;i++)
	{
		setValue((*iter).outputmaps[i][0],x[i],Len,Len);//28 is the size of input image witch we can change
		//(*iter).outputmaps[i][0] = x[i];
	}
}
// & for change the value in m_Layers
void CNN::setConvOutput(Layer &layer, Layer &lastLayer)
{
	int mapNum = layer.getOutMapNum();
	int lastMapNum = lastLayer.getOutMapNum();
	int last_x=lastLayer.getMapSize().x;
	int last_y=lastLayer.getMapSize().y;
	int kernel_x=layer.getKernelSize().x;
	int kernel_y=layer.getKernelSize().y;
	int x = layer.getMapSize().x;
	int y = layer.getMapSize().y;
	double** sum, **sumNow;
	sum = new double*[x];
	sumNow = new double*[x];
	for (int nn=0; nn<x;nn++)
	{
		sum[nn] = new double[y];
		sumNow[nn] = new double[y];
	}
	for(int numBatch=0; numBatch<batchSize;numBatch++)
	{	
		for (int i =0; i<mapNum; i++)
		{	
			
			for (int j=0; j<lastMapNum; j++)
			{
				double **lastMap;
				lastMap= lastLayer.outputmaps[numBatch][j];
				
				if(j==0)
				{	
					convnValid(lastMap, layer.kernel[j][i], last_x, last_y, kernel_x, kernel_y, sum);
					//sum = convnValid(lastMap,layer.kernel[j][i],lastLayer.getMapSize().x, lastLayer.getMapSize().y, layer.getKernelSize().x,layer.getKernelSize().y);//each time we calculate one image of batch and also calculate sigmoid 
				}
				// get convnValid value plus previous values
				else
				{  
					convnValid(lastMap, layer.kernel[j][i], last_x, last_y, kernel_x, kernel_y, sumNow);//���ξ���ֵ����sumNow��
					ArrayPlus(sumNow,sum,x,y);// sumNow ��sum��� �������sum��
					//sum = ArrayPlus(convnValid(lastMap,layer.kernel[j][i],lastLayer.getMapSize().x, lastLayer.getMapSize().y, layer.getKernelSize().x,layer.getKernelSize().y) ,sum, layer.getMapSize().x, layer.getMapSize().y);
				}
			}
			Sigmoid(sum,layer.bias[i],x, y);
			//layer.outputmaps[numBatch][i] = sum;// ��ô�� ����ֱ��ָ����sum�ռ䣬sum��delete��ֻʣ��sum�ĵ�ֵַ�����������ǿյģ�����mapָ����һ��sum��ַ���������ǿյģ�Ҳ�������ݱ���������
			setValue(layer.outputmaps[numBatch][i],sum, x, y);// ��������ʱb���õ�y��ֻ��һά��ַ̫Σ�գ�����ֱ��ָ���������ֵ����
			
		}
	}
	for (int i=0; i<x; i++)
	{
		delete []sum[i];
		delete []sumNow[i];
	}
	delete []sum;
	delete []sumNow;
}

void CNN::setSampOutput(Layer &layer, Layer &lastLayer)
{
	int lastMapNum = lastLayer.getOutMapNum();
	int last_x=lastLayer.getMapSize().x;
	int last_y=lastLayer.getMapSize().y;
	int x = layer.getMapSize().x;
	int y = layer.getMapSize().y;
	double **sampMatrix, **lastMap;
	size scaleSize;
	sampMatrix = new double *[layer.getMapSize().x];
	for (int j=0; j<layer.getMapSize().x;j++)
	{
		sampMatrix[j] = new double[layer.getMapSize().y];
	}
	for(int num=0; num<batchSize;num++)
	{
		for (int i=0; i<lastMapNum; i++)
		{
			lastMap = lastLayer.outputmaps[num][i];
			scaleSize = layer.getScaleSize();
			scaleMatrix(lastMap,scaleSize,last_x,last_y,sampMatrix);
			//sampMatrix = scaleMatrix(lastMap,scaleSize,lastLayer.getMapSize().x,lastLayer.getMapSize().y);
			setValue(layer.outputmaps[num][i],sampMatrix,x,y);
			//layer.outputmaps[num][i]=sampMatrix;
		}
	}
	for (int i=0; i<x; i++)
	{
		delete []sampMatrix[i];
	}
	delete []sampMatrix;
}

void CNN::setOutLayerErrors(double*** x,double** y)
{
	layers :: iterator iter = m_layers.end();
	iter--;
	int mapNum = (*iter).getOutMapNum();
	double meanError=0.0, maxError = 0.0;
	for(int numBatch=0; numBatch<batchSize;numBatch++)
	{		
		for (int m=0; m<mapNum; m++)
		{
			double outmaps = (*iter).outputmaps[numBatch][m][0][0];
			double target = y[numBatch][m];
			(*iter).setError(numBatch,m,0,0,DSIGMOID(outmaps)*(target-outmaps));
// 			meanError += abs(target-outmaps);
// 			if (abs(target-outmaps)>maxError)
// 			{
// 				maxError = abs(target-outmaps);
// 			}
 		}
 	}
// 	cout<<"Mean error of each mini batch: "<<meanError<<endl;	//����ÿ��batch������
// 	cout<<"The max error of one output in mini batch: "<<maxError<<endl;
}

void CNN::setHiddenLayerErrors()
{
	layers :: iterator iter= m_layers.end();
	iter = iter - 2;
	for (iter; iter > m_layers.begin(); iter--)
	{
// 		switch((*iter).getType())
// 		{
// 		case 'S':
// 			setSampErrors((*iter),(*(iter+1)));
// 			break;
// 		case 'C':
// 			setConvErrors((*iter),(*(iter+1)));// �����޶������һ�����ز���벻���Ǿ����㣬������output��Ҳ�Ǿ����㣬������Ҫ��������������ǲ�����
// 			break;
// 		default:// input layer has no error
// 			break;
// 		}
		switch((*(iter+1)).getType())
		{
		case 'C':
			setSampErrors((*iter), (*(iter+1)));
			break;
		case 'S':
			setConvErrors((*iter), (*(iter+1)));
			break;
		case 'O':
			setSampErrors((*iter), (*(iter+1)));//O�����conv�� ֻ����kernelsize����Ϊ1
			break;
		default:
			break;
		}
	}

}
//��һ��Ϊ������
void CNN::setSampErrors(Layer &layer, Layer &nextLayer)
{
	int mapNum = layer.getOutMapNum();
	int nextMapNum = nextLayer.getOutMapNum();
	int next_x=nextLayer.getMapSize().x;
	int next_y=nextLayer.getMapSize().y;
	int kernel_x=nextLayer.getKernelSize().x;
	int kernel_y=nextLayer.getKernelSize().y;
	int x = layer.getMapSize().x;
	int y = layer.getMapSize().y;
	double **nextError;
	double **kernel;
	double **sum, **rotMatrix, **sumNow;
	//initialize
	sum = new double *[x];
	for (int k=0; k<x; k++)
	{
		sum[k] = new double[y];
	}
	rotMatrix = new double *[kernel_x];
	for (int kk=0; kk<kernel_x; kk++)
	{
		rotMatrix[kk] = new double[kernel_y];
	}
	sumNow =  new double *[x];
	for (int k=0; k<x; k++)
	{
		sumNow[k] = new double[y];
	}
	double **extendMatrix;
	// ��չ ����ʼ�� ȫ0����
	int m = next_x, n = next_y, km = kernel_x, kn = kernel_y;
	extendMatrix = new double*[m + 2 * (km - 1)];
	for (int k=0; k<m+2*(km-1); k++)
	{
		extendMatrix[k] = new double[n+2*(kn-1)];
		for (int a=0; a<n+2*(kn-1); a++)
		{
			extendMatrix[k][a] = 0.0;
		}
	}
	//calculate
	for (int numBatch=0; numBatch<batchSize; numBatch++)
	{
		for (int i=0; i<mapNum; i++)
		{
			
			for (int j=0; j<nextMapNum; j++)
			{
				
				nextError = nextLayer.getError(numBatch,j);
				kernel = nextLayer.getKernel(i,j);
				if (j==0)
				{
 					rot180(kernel, kernel_x, kernel_y, rotMatrix);
 					convnFull(nextError, rotMatrix, next_x, next_y, kernel_x, kernel_y, sum, extendMatrix);
					//sum = convnFull(nextError, rot180(kernel, nextLayer.getKernelSize().x, nextLayer.getKernelSize().y), nextLayer.getMapSize().x, nextLayer.getKernelSize().y,nextLayer.getKernelSize().x, nextLayer.getKernelSize().y);
				}
				else
				{
					rot180(kernel, kernel_x, kernel_y, rotMatrix);
					convnFull(nextError, rotMatrix, next_x, next_y, kernel_x, kernel_y, sumNow, extendMatrix);
					ArrayPlus(sumNow, sum, x, y);
					//sum = ArrayPlus(convnFull(nextError, rot180(kernel, nextLayer.getKernelSize().x, nextLayer.getKernelSize().y), nextLayer.getMapSize().x, nextLayer.getKernelSize().y,nextLayer.getKernelSize().x, nextLayer.getKernelSize().y), sum, layer.getMapSize().x, layer.getMapSize().y);
				}
				
			}
			layer.setError(numBatch, i, sum, x, y);
		}
	}
	for (int i=0; i<x; i++)
	{
		delete []sum[i];
		delete []sumNow[i];
	}
	for (int i=0; i<kernel_x; i++)
	{
		delete []rotMatrix[i];
	}
	for (int i=0; i<m + 2 * (km - 1); i++)
	{
		delete []extendMatrix[i];
	}
	delete []rotMatrix;
	delete []sumNow;
	delete []sum;
	delete []extendMatrix;
}

void CNN::setConvErrors(Layer &layer, Layer &nextLayer)
{   // ����Ϊ�����㣬��һ��Ϊ�����㣬�������map������ͬ����һ��mapֻ����һ���һ��map���ӣ�
	// ���ֻ�轫��һ��Ĳв�kronecker��չ���õ������
	int mapNum = layer.getOutMapNum();
	int x = layer.getMapSize().x;
	int y = layer.getMapSize().y;
	int nx = nextLayer.getMapSize().x;
	int ny = nextLayer.getMapSize().y;
	double **nextError;
	double **map;
	double **outMatrix, **kroneckerMatrix;
	size scale;
	outMatrix = new double *[x];
	kroneckerMatrix = new double *[x];
	for (int i=0; i<x; i++)
	{
		outMatrix[i] = new double [y];
		kroneckerMatrix[i] = new double[y];
	}
	for (int numBatch=0; numBatch<batchSize; numBatch++)
	{
		for (int m=0; m<mapNum; m++)
		{
			scale = nextLayer.getScaleSize();
			nextError = nextLayer.getError(numBatch,m);
			map = layer.outputmaps[numBatch][m];
			//������ˣ����Եڶ��������ÿ��Ԫ��value����1-value����
			matrixDsigmoid(map, x, y, outMatrix);
			kronecker(nextError, scale, nx, ny, kroneckerMatrix);
			matrixMultiply(outMatrix, kroneckerMatrix, x, y);
			//outMatrix = matrixMultiply(outMatrix, kronecker(nextError, scale, nx, ny), x, y);
			layer.setError(numBatch, m, outMatrix, x, y);
		}
	}
	for (int i=0; i<x; i++)
	{
		delete []outMatrix[i];
		delete []kroneckerMatrix[i];
	}
	delete []outMatrix;
	delete []kroneckerMatrix;
}

void CNN::updateParas()
{
	layers :: iterator iter = m_layers.begin();
	iter++;
	for (iter; iter<m_layers.end(); iter++)
	{
		switch((*iter).getType())
		{
		case 'C':
			updateKernels(*iter,*(iter-1));
			updateBias(*iter);
			break;
		case 'O':
			updateKernels(*iter,*(iter-1));
			updateBias(*iter);
			break;
		default:
			break;
		}
	}
}
	

void CNN::updateBias(Layer &layer)
{
	double ****errors = layer.errors;
	int mapNum = layer.getOutMapNum();
	int x = layer.getMapSize().x;
	int y = layer.getMapSize().y;
	double **error;
	error = new double *[x];
	for (int i=0; i<x; i++)
	{
		error[i] = new double[y];
	}
	for (int j=0; j<mapNum; j++)
	{
		sum(errors,j, x, y, batchSize, error);
		double deltaBias = sum(error, layer.getMapSize().x, layer.getMapSize().y)/batchSize;
		double bias = layer.bias[j] + ALPHA * deltaBias;
		layer.bias[j] = bias;
	}
	for (int i=0; i<x; i++)
	{
		delete []error[i];
	}
	delete []error;
}

void CNN::updateKernels(Layer &layer, Layer &lastLayer)
{
	int mapNum = layer.getOutMapNum();
	int lastMapNum = lastLayer.getOutMapNum();
	int kernel_x = layer.getKernelSize().x;
	int kernel_y = layer.getKernelSize().y;
	int last_x = lastLayer.getMapSize().x;
	int last_y = lastLayer.getMapSize().y;
	int x = layer.getMapSize().x;
	int y = layer.getMapSize().y;
	double** deltakernel, **deltaNow;
	deltakernel = new double*[kernel_x];
	deltaNow = new double*[kernel_x];
	for (int ii=0; ii<kernel_x; ii++)
	{
		deltakernel[ii]=new double[kernel_y];
		deltaNow[ii] =new double[kernel_y];
	}
	for (int j=0; j<mapNum; j++)
	{
		for (int i=0; i<lastMapNum; i++)
		{
			for (int r=0; r<batchSize; r++)
			{
				double** error = layer.errors[r][j];
				if(r==0){
					convnValid(lastLayer.outputmaps[r][i],error,last_x, last_y, x, y, deltakernel);
					//deltakernel = convnValid(lastLayer.outputmaps[r][i],error, lastLayer.getMapSize().x, lastLayer.getMapSize().y, layer.getMapSize().x, layer.getMapSize().y);
				}
				else{
					convnValid(lastLayer.outputmaps[r][i],error,last_x, last_y, x, y, deltaNow);
					ArrayPlus( deltaNow, deltakernel,kernel_x, kernel_y);
					//deltakernel = ArrayPlus(convnValid(lastLayer.outputmaps[r][i],error, lastLayer.getMapSize().x, lastLayer.getMapSize().y, layer.getMapSize().x, layer.getMapSize().y), deltakernel,layer.getKernelSize().x, layer.getKernelSize().y);
				}
			}
			ArrayDivide(deltakernel, batchSize, layer.getKernelSize().x, layer.getKernelSize().y);
			ArrayMultiply(deltakernel, ALPHA,layer.getKernelSize().x, layer.getKernelSize().y);
			ArrayPlus(deltakernel , layer.kernel[i][j], layer.getKernelSize().x, layer.getKernelSize().y);
		}
	}
	for (int i=0; i<kernel_x; i++)
	{
		delete []deltakernel[i];
		delete []deltaNow[i];
	}
	delete []deltakernel;
	delete []deltaNow;
}

void CNN::test(double*** test_x,double** test_label, int number)
{
	cout<<"Start test"<<endl;
	int fause=0, predict,real;
	int Num = number/batchSize;
	double ***Test;
	//setBatchsize(1);
	Test = new double **[batchSize];
	for (int i=0; i<Num; i++)
	{
// 		for (int j=0; j<batchSize; j++)
// 		{
// 			Test[j] = test_x[i*batchSize+j];
// 		}
		for ( int j=0; j<batchSize; j++)
		{
			if (i == 0)
			{
				Test[j] = new double *[Len];
			}
			
			for (int m=0; m<Len; m++)
			{
				if (i==0)
				{
					Test[j][m] = new double [Len];
				}			
				for (int n=0; n<Len; n++)
				{
					Test[j][m][n] = test_x[i*batchSize+j][m][n];
				}
			}
		}
		forward(Test);
		layers ::iterator iter = m_layers.end();
		iter--;
		for (int ii=0; ii<batchSize; ii++)
		{
			predict = findIndex((*iter).outputmaps[ii]);
			real = findIndex(test_label[i*batchSize+ii]);
			if(predict != real)
			{
				fause++;
			}
		}
	}
	
	cout<<"Finish test"<<endl;
	cout<<"Error predict number: "<< fause<<endl;
	cout<<"Rate of error :"<< 1.0*fause/number<<endl;

	for (int i=0; i<batchSize; i++)
	{
		for (int j=0; j<Len; j++)
		{
			delete []Test[i][j];
		}
		delete []Test[i];
	}
	delete []Test;
}

void CNN::test_o(double*** test_x,double** test_label, int number)
{
	layers::iterator iter = m_layers.begin();
	iter++;
	double ****m_kernel=(*iter).kernel;
	double *m_bias=(*iter).bias;
	int i,j,k,l;
	FILE *fp;
	//load the first conv layer weight and bias
	 if((fp=fopen("conv_layer1_kernel.txt","rt"))==NULL)
	 {
		  printf("cannot open conv1_w\n");
	 }
	 for(i=0;i<6;i++){
		 for(j=0;j<5;j++){
			 for(k=0;k<5;k++){
				  fscanf(fp,"%lf",&m_kernel[0][i][j][k]);
				  fscanf(fp,"\n");
			 }
		 }
	 }
	 fclose(fp);
	 if((fp=fopen("conv_layer1_bias.txt","rt"))==NULL)
	 {
		  printf("cannot open conv1_b\n");
	 }
	  for(i=0;i<6;i++)
	  {
	       fscanf(fp,"%lf",&m_bias[i]);
		   fscanf(fp,"\n");
	  }
	  fclose(fp);
	  //load the second conv layer weight and bias
	  iter++;iter++;
      m_kernel=(*iter).kernel;
	  m_bias=(*iter).bias;
	   if((fp=fopen("conv_layer2_kernel.txt","rt"))==NULL)
	 {
		  printf("cannot open conv2_w\n");
	 }
	 for(i=0;i<6;i++){
		 for(j=0;j<12;j++){
			 for(k=0;k<5;k++){
				 for(l=0;l<5;l++){
					  fscanf(fp,"%lf",&m_kernel[i][j][k][l]);
					  fscanf(fp,"\n");
				 }
			 }
		 }
	 }
	 fclose(fp);
	 if((fp=fopen("conv_layer2_bias.txt","rt"))==NULL)
	 {
		  printf("cannot open conv2_b\n");
	 }
	  for(i=0;i<12;i++)
	  {
	       fscanf(fp,"%lf",&m_bias[i]);
		   fscanf(fp,"\n");
	  }
	  fclose(fp);
	  //load the third conv layer weight and bias
	  iter++;iter++;
      m_kernel=(*iter).kernel;
	  m_bias=(*iter).bias;
	   if((fp=fopen("conv_layer3_kernel.txt","rt"))==NULL)
	 {
		  printf("cannot open conv3_w\n");
	 }
	 for(i=0;i<12;i++){
		 for(j=0;j<10;j++){
			 for(k=0;k<4;k++){
				 for(l=0;l<4;l++){
					  fscanf(fp,"%lf",&m_kernel[i][j][k][l]);
					  fscanf(fp,"\n");
				 }
			 }
		 }
	 }
	 fclose(fp);
	 if((fp=fopen("conv_layer3_bias.txt","rt"))==NULL)
	 {
		  printf("cannot open conv3_b\n");
	 }
	  for(i=0;i<10;i++)
	  {
	       fscanf(fp,"%lf",&m_bias[i]);
		   fscanf(fp,"\n");
	  }
	  fclose(fp);

	 //get test data
	int Num = number/batchSize;
	double ***Test;
	//setBatchsize(1);
	Test = new double **[batchSize];
	for (int i=0; i<Num; i++)
	{
// 		for (int j=0; j<batchSize; j++)
// 		{
// 			Test[j] = test_x[i*batchSize+j];
// 		}
		for ( int j=0; j<batchSize; j++)
		{
			if (i == 0)
			{
				Test[j] = new double *[Len];
			}
			
			for (int m=0; m<Len; m++)
			{
				if (i==0)
				{
					Test[j][m] = new double [Len];
				}			
				for (int n=0; n<Len; n++)
				{
					Test[j][m][n] = test_x[i*batchSize+j][m][n];
				}
			}
		}	
		forward(Test);
		layers ::iterator iter = m_layers.end();
		iter--;
		for (int ii=0; ii<batchSize; ii++)
		{
			int predict = findIndex((*iter).outputmaps[ii]);
			int real = findIndex(test_label[i*batchSize+ii]);
			cout<<predict<<" "<<real<<endl;
		}
	}
}

void CNN::load_weight()
{
	layers::iterator iter = m_layers.begin();
	iter++;
	double ****m_kernel=(*iter).kernel;
	double *m_bias=(*iter).bias;
	int i,j,k,l;
	FILE *fp;
	//load the first conv layer weight and bias
     if((fp=fopen("./bin/conv_layer1_kernel.txt","rt"))==NULL)
	 {
		  printf("cannot open conv1_w\n");
	 }
	 for(i=0;i<6;i++){
		 for(j=0;j<5;j++){
			 for(k=0;k<5;k++){
				  fscanf(fp,"%lf",&m_kernel[0][i][j][k]);
				  fscanf(fp,"\n");
			 }
		 }
	 }
	 fclose(fp);
     if((fp=fopen("./bin/conv_layer1_bias.txt","rt"))==NULL)
	 {
		  printf("cannot open conv1_b\n");
	 }
	  for(i=0;i<6;i++)
	  {
	       fscanf(fp,"%lf",&m_bias[i]);
		   fscanf(fp,"\n");
	  }
	  fclose(fp);
	  //load the second conv layer weight and bias
	  iter++;iter++;
      m_kernel=(*iter).kernel;
	  m_bias=(*iter).bias;
       if((fp=fopen("./bin/conv_layer2_kernel.txt","rt"))==NULL)
	 {
		  printf("cannot open conv2_w\n");
	 }
	 for(i=0;i<6;i++){
		 for(j=0;j<12;j++){
			 for(k=0;k<5;k++){
				 for(l=0;l<5;l++){
					  fscanf(fp,"%lf",&m_kernel[i][j][k][l]);
					  fscanf(fp,"\n");
				 }
			 }
		 }
	 }
	 fclose(fp);
     if((fp=fopen("./bin/conv_layer2_bias.txt","rt"))==NULL)
	 {
		  printf("cannot open conv2_b\n");
	 }
	  for(i=0;i<12;i++)
	  {
	       fscanf(fp,"%lf",&m_bias[i]);
		   fscanf(fp,"\n");
	  }
	  fclose(fp);
	  //load the third conv layer weight and bias
	  iter++;iter++;
      m_kernel=(*iter).kernel;
	  m_bias=(*iter).bias;
       if((fp=fopen("./bin/conv_layer3_kernel.txt","rt"))==NULL)
	 {
		  printf("cannot open conv3_w\n");
	 }
	 for(i=0;i<12;i++){
		 for(j=0;j<10;j++){
			 for(k=0;k<4;k++){
				 for(l=0;l<4;l++){
					  fscanf(fp,"%lf",&m_kernel[i][j][k][l]);
					  fscanf(fp,"\n");
				 }
			 }
		 }
	 }
	 fclose(fp);
     if((fp=fopen("./bin/conv_layer3_bias.txt","rt"))==NULL)
	 {
		  printf("cannot open conv3_b\n");
	 }
	  for(i=0;i<10;i++)
	  {
	       fscanf(fp,"%lf",&m_bias[i]);
		   fscanf(fp,"\n");
	  }
	  fclose(fp);
	  cout<<"CNN_load_weight success"<<endl;
}

//int CNN::test_pic(char* name)
//{
//	int predict;
//	batchSize=1;
//	Mat img;
//	int sum=0;
//	img=imread(name,0);
//	if(img.empty())
//		return -1;
//	resize(img,img,Size(Len,Len));
//	uchar *p=img.data;
//	double ***test;
//	test=new double**[1];
//	test[0]=new double*[Len];
//	for(int i=0;i<Len;i++){
//		test[0][i]=new double[Len];
//	}
//	for(int i=0;i<Len;i++){
//		for(int j=0;j<Len;j++){
//			test[0][i][j]=(double)p[i*Len+j];
//			sum+=(p[i*Len+j]>120);
//		}
//	}
//	cout<<sum<<endl;
//	forward(test);
//	layers ::iterator iter = m_layers.end();
//	iter--;
//	for (int ii=0; ii<batchSize; ii++)
//	{
//		predict = findIndex((*iter).outputmaps[ii]);
//	}
//	return predict;
//}

int CNN::test_Pic(Pic<uchar> img)
{
	int predict;
	batchSize=1;
	int sum=0;
	Pic<uchar> img2;
	m_resize(img,img2,Len,Len);
	uchar *p=img2.data;
	double ***test;
	test=new double**[1];
	test[0]=new double*[Len];
	for(int i=0;i<Len;i++){
		test[0][i]=new double[Len];
	}
	for(int i=0;i<Len;i++){
		for(int j=0;j<Len;j++){
			test[0][i][j]=(double)p[i*Len+j];
			sum+=(p[i*Len+j]>100);
		}
	}
	if(sum<20)
		return -1;
	forward(test);
	layers ::iterator iter = m_layers.end();
	iter--;
	for (int ii=0; ii<batchSize; ii++)
	{
		predict = findIndex((*iter).outputmaps[ii]);
	}
	return predict;
}

//int CNN::test_frame(Mat img)
//{
//	int predict;
//	batchSize=1;
//	Mat img2;
//	if(img.empty())
//		return -1;
//	resize(img,img2,Size(Len,Len));
//	uchar *p=img2.data;
//	double ***test;
//	test=new double**[1];
//	test[0]=new double*[Len];
//	for(int i=0;i<Len;i++){
//		test[0][i]=new double[Len];
//	}
//	for(int i=0;i<Len;i++){
//		for(int j=0;j<Len;j++){
//			test[0][i][j]=(double)p[i*Len+j];
//		}
//	}
//	forward(test);
//	layers ::iterator iter = m_layers.end();
//	iter--;
//	for (int ii=0; ii<batchSize; ii++)
//	{
//		predict = findIndex((*iter).outputmaps[ii]);
//	}
//	return predict;
//}
//
//vector<int> CNN::test_frame(vector<Mat> imgs)
//{
//	vector<int> predict;
//	Mat img2;
//	int num=imgs.size();
//	batchSize=num;
//	if(num==0){
//		predict.push_back(-1);
//		return predict;
//	}
//	double ***test;
//	test=new double**[num];
//	test[0]=new double*[Len];
//	for(int i=0;i<Len;i++){
//		test[0][i]=new double[Len];
//	}
//	for(int n=0;n<num;n++){
//		resize(imgs[n],img2,Size(Len,Len));
//		uchar *p=img2.data;
//		for(int i=0;i<Len;i++){
//			for(int j=0;j<Len;j++){
//				test[n][i][j]=(double)p[i*Len+j];
//			}
//		}
//	}
//	forward(test);
//	layers ::iterator iter = m_layers.end();
//	iter--;
//	for (int ii=0; ii<batchSize; ii++)
//	{
//		predict.push_back(findIndex((*iter).outputmaps[ii]));
//	}
//	return predict;
//}

void CNN::getlayerweight()
{
	layers::iterator iter = m_layers.begin();
	iter++;
	double ****m_kernel=(*iter).kernel;
	double *m_bias=(*iter).bias;
	FILE *fp_fc_w,*fp_fc_b;
    if(fp_fc_w=fopen("conv_layer1_kernel.txt","w"))
		cout<<"success"<<endl;
	else
		cout<<"false"<<endl;
	for(int i=0;i<6;i++)
		for(int j=0;j<5;j++)
			for(int k=0;k<5;k++)
		        fprintf(fp_fc_w,"%lf\n",m_kernel[0][i][j][k]);
	fclose(fp_fc_w);
	fp_fc_b=fopen("conv_layer1_bias.txt","w");
	for(int i=0;i<6;i++)
		fprintf(fp_fc_w,"%lf\n",m_bias[i]);
	fclose(fp_fc_b);

	iter++;iter++;
	 m_kernel=(*iter).kernel;
	 m_bias=(*iter).bias;
    if(fp_fc_w=fopen("conv_layer2_kernel.txt","w"))
		cout<<"success"<<endl;
	else
		cout<<"false"<<endl;
	for(int i=0;i<6;i++)
		for(int j=0;j<12;j++)
			for(int k=0;k<5;k++)
				for(int l=0;l<5;l++)
					fprintf(fp_fc_w,"%lf\n",m_kernel[i][j][k][l]);
	fclose(fp_fc_w);
	fp_fc_b=fopen("conv_layer2_bias.txt","w");
	for(int i=0;i<12;i++)
		fprintf(fp_fc_w,"%lf\n",m_bias[i]);
	fclose(fp_fc_b);

	iter++;iter++;
	 m_kernel=(*iter).kernel;
	 m_bias=(*iter).bias;
    if(fp_fc_w=fopen("conv_layer3_kernel.txt","w"))
		cout<<"success"<<endl;
	else
		cout<<"false"<<endl;
	for(int i=0;i<12;i++)
		for(int j=0;j<10;j++)
			for(int k=0;k<4;k++)
				for(int l=0;l<4;l++)
					fprintf(fp_fc_w,"%lf\n",m_kernel[i][j][k][l]);
	fclose(fp_fc_w);
	fp_fc_b=fopen("conv_layer3_bias.txt","w");
	for(int i=0;i<10;i++)
		fprintf(fp_fc_w,"%lf\n",m_bias[i]);
	fclose(fp_fc_b);

}