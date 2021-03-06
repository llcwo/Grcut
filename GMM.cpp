#include "GMM.h"
//更新GMM参数
void GMM::update() 
{
	for (int c = 0; c < componentsCount; c++)
	{
		if (counts[c] == 0) {
			coefs[c] = 0;
			continue;
		}
		//第c个高斯模型的贡献
		coefs[c] = (double)counts[c] / (double)samplesCount;

		//计算第c个高斯模型的期望
		int meanBase = c * channelsCount;
		for (int i = 0; i < channelsCount; i++)
		{
			mean[meanBase + i] = sum[c][i] / counts[c];
		}

		//计算第c个高斯模型的协方差
		double *co = cov + c * channelsCount * channelsCount;
		for (int i = 0; i < channelsCount; i++)
			for (int j = 0; j < channelsCount; j++)
			{
				co[i * channelsCount + j] =
					product[c][i][j] / counts[c] - mean[meanBase + i] * mean[meanBase + j];

			}
		
		//计算第c个高斯模型的协方差逆矩阵和行列式
		det[c] = co[0] * (co[4] * co[8] - co[5] * co[7])
			- co[1] * (co[3] * co[8] - co[5] * co[6])
			+ co[2] * (co[3] * co[7] - co[4] * co[6]);
		//为了计算逆矩阵，为行列式小于等于0的矩阵对角线增加噪声
		if (det[c] <= std::numeric_limits<double>::epsilon())
		{
			co[0] += 0.001;
			co[4] += 0.001;
			co[8] += 0.001;

			det[c] = co[0] * (co[4] * co[8] - co[5] * co[7])
				- co[1] * (co[3] * co[8] - co[5] * co[6])
				+ co[2] * (co[3] * co[7] - co[4] * co[6]);
		}
		CV_Assert(det[c] > std::numeric_limits<double>::epsilon());
		
		//计算逆
		inv[c][0][0] =  (co[4]*co[8] - co[5]*co[7]) / det[c];  
        inv[c][1][0] = -(co[3]*co[8] - co[5]*co[6]) / det[c];  
        inv[c][2][0] =  (co[3]*co[7] - co[4]*co[6]) / det[c];  
        inv[c][0][1] = -(co[1]*co[8] - co[2]*co[7]) / det[c];  
        inv[c][1][1] =  (co[0]*co[8] - co[2]*co[6]) / det[c];  
        inv[c][2][1] = -(co[0]*co[7] - co[1]*co[6]) / det[c];  
        inv[c][0][2] =  (co[1]*co[5] - co[2]*co[4]) / det[c];  
        inv[c][1][2] = -(co[0]*co[5] - co[2]*co[3]) / det[c];  
        inv[c][2][2] =  (co[0]*co[4] - co[1]*co[3]) / det[c];  
	}

}
//增加一个像素点
void GMM::add(int c, const Vec3d pixel)
{
	for (int i = 0; i < channelsCount; i++) 
		sum[c][i] += pixel[i];

	for (int i = 0; i < channelsCount; i++)
		for (int j = 0; j < channelsCount;j++) 
		{
			product[c][i][j] += pixel[i] * pixel[j];
		}
	counts[c]++;
	samplesCount++;
}

//初始化，全部置零
void GMM::init() 
{
	memset(sum, 0, sizeof(double) * componentsCount * channelsCount);
	memset(product, 0, sizeof(double) * componentsCount * channelsCount * channelsCount);
	memset(counts, 0, sizeof(double) * componentsCount);
	samplesCount = 0;
}

GMM::GMM(Mat& _model)
{
	const int modelSize = 1 + channelsCount + channelsCount * channelsCount;
	if (_model.empty()) {
		_model.create(1, modelSize * componentsCount, CV_64FC1);
		_model.setTo(Scalar(0));
	}
	else if (_model.type() != CV_64FC1 || _model.rows != 1 || _model.cols != modelSize * componentsCount)
		CV_Error(CV_StsBadArg, "_model must be in 1 * 13 double type.");
	model = _model;

	//排列顺序： 1个权重，3个均值，9个协方差
	coefs = model.ptr<double>(0);
	mean = coefs + componentsCount;
	cov = mean + channelsCount * componentsCount;
}