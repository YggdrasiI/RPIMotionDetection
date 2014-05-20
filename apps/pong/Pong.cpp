#include <cstdio>
#include <cmath>

#include "Graphics.h"
#include "Pong.h"

/* This array is defined in the shader description file pong.c 
 * Update this values to update the shown score or both players.
 * Allowed values are 0-9.
 * */
extern "C" int score[2];

Pong::Pong(float radius, float aspect): m_aspect(aspect) {
	m_radius[0] = radius;
	m_radius[1] = radius*m_aspect;
	reset();
}


void Pong::updatePosition(){
	m_position[0] += m_velocity[0];
	m_position[1] += m_velocity[1];
	//check boundaries
#define SKIP 0.0
	for( int i=0; i<2; ++i){
		const float border = 1.0-SKIP-m_radius[i]; 
		if( m_position[i] > border ){
			m_position[i] = 2*border - m_position[i];
			m_velocity[i] = -m_velocity[i];
			Col(255-i*255,0,i*255);
			if( i == 0){
				score[1] = (score[1]+1)%10;
			}
		}else if( m_position[i] < -border ){
			m_position[i] = -2*border - m_position[i];
			m_velocity[i] = -m_velocity[i];
			Col(255-i*255,0,i*255);
			if( i == 0){
				score[0] = (score[0]+1)%10;
			}
		}else{
			linearColor(1.03, 0.03);
		}
	}
}

void Pong::reset(){
	m_position[0] = 0.0; m_position[1] = 0.0;
	m_velocity[0] = 0.01; m_velocity[1] = 0.005;
	m_score[0] = 0; m_score[1] = 0;
	m_color[0] = 1.0; m_color[1] = 1.0; m_color[2] = 1.0;
}

const float* const Pong::getPosition(){
	return &m_position[0];
}

const float* const Pong::getVelocity(){
	return &m_velocity[0];
}

void Pong::setPosition(float x, float y){
	m_position[0] = x;
	m_position[1] = y;
}
void Pong::setVelocity(float x, float y){
	m_velocity[0] = x;
	m_velocity[1] = y;
}
void Pong::scaleVelocity(float x, float y){
	m_velocity[0] *= x;
	m_velocity[1] *= y;
}

void Pong::setRadius(float r){
	m_radius[0] = r;
	m_radius[1] = r*m_aspect;
}

void Pong::setAspect(float a){
	m_aspect = a;
	m_radius[1] = m_radius[0]*m_aspect;
}

void Pong::drawBall(){
	//printf("Draw ball on position %f %f, (r=%f)\n", m_position[0], m_position[1], m_radius[0]);
	DrawBlobRect( m_color[0], m_color[1], m_color[2],
			m_position[0]-m_radius[0],
			m_position[1]-m_radius[1],
			m_position[0]+m_radius[0],
			m_position[1]+m_radius[1],
			NULL);
}


void Pong::linearColor(float factor, float abs){
	for(int i=0; i<3; ++i){
		m_color[i] = fmax(0.0, fmin( factor*m_color[i]+abs, 1.0));
	}
}


bool Pong::checkCollision(int width, int height, std::vector<cBlob> &blobs){
	//Transform pong position from [-1,1]² into integer space [0,width]x[0,height]
	//Moreover, flipping horizontal because orientation differs
	int iPos[2] = { width - 0.5*(m_position[0]+1.0)*width, 0.5*(m_position[1]+1.0)*height };
	int iRadius[2] = { m_radius[0]*width, m_radius[1]*height };

	for (int i = 0; i < blobs.size(); i++) {
		cBlob &b = blobs[i];
		if( b.min.x /*- iRadius[0]*/ < iPos[0] &&	
				b.max.x /*+ iRadius[0]*/ > iPos[0] &&	
				b.min.y /*- iRadius[1]*/ < iPos[1] &&	
				b.max.y /*+ iRadius[1]*/ > iPos[1] ){
			//reflect pong, if x-velocity matches to x-position of blob
			//Note, velocity is inverted due other orientation of coordinate system
			if( iPos[0] < width/3 && m_velocity[0] > 0 ){
				scaleVelocity(-1.0,1.0);
				Col(0,255,0);
				printf("Hit 1!\n");
			}else if( iPos[0] > 2*width/3 && m_velocity[0] < 0 ){
				Col(0,255,0);
				scaleVelocity(-1.0,1.0);
				printf("Hit 2!\n");
			}

			//optional use blob offset for change of y-velocity 
		}

	}
	return false;
}
