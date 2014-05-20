#ifndef PONG_H
#define PONG_H

#include <stdlib.h>
#include <vector>

#include "../../Blob.h"

class Pong{
	private:
		/* Position varies in [-1,1]^2 and ignores the aspect ratio */
		float m_position[2];
		float m_velocity[2];
		int m_score[2];
		float m_radius[2];
		float m_aspect;
		float m_color[3];
#define Col(r,g,b) {m_color[0]=(r)/255.0; m_color[1]=(g)/255.0; m_color[2]=(b)/255.0;}

		//C=>factor*C+abs
		void linearColor(float factor, float abs);

	public:
		Pong(float radius, float aspect);
		void updatePosition();
		void reset();
		const float* const getPosition();
		const float* const getVelocity();
		const int* const getScore();
		void setPosition(float x, float y);
		void setVelocity(float x, float y);
		void scaleVelocity(float x, float y);
		void changeVelocity(float x, float y);
		void setRadius(float r);
		void setAspect(float a);
		void drawBall();

		//width and height are the dimensions of the blob rect coordinates.
		bool checkCollision(int width, int height,  std::vector<cBlob> &blobs);

};

#endif
