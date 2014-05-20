attribute vec2 vertex;
uniform vec2 offset;
uniform vec2 scale;
varying vec2 texcoord;
varying vec2 scorecoordLeft;
varying vec2 scorecoordRight;
uniform vec2 score;
uniform vec4 scorePosLeft;
uniform vec4 scorePosRight;
void main(void) {
   /*texcoord = 0.5 * (1.0 - vertex );*/
   texcoord = vertex.xy;
   vec2 leftWH = vec2( scorePosLeft.z-scorePosLeft.x, scorePosLeft.a - scorePosLeft.y );
   vec2 leftPos = vec2( (texcoord.x-scorePosLeft.x)/(leftWH.x), (texcoord.y-scorePosLeft.y)/(leftWH.y) );
   scorecoordLeft.x = score.x*0.1+leftPos.x*0.1;
   scorecoordLeft.y = leftPos.y;
   vec2 rightWH = vec2( scorePosRight.z-scorePosRight.x, scorePosRight.a - scorePosRight.y );
   vec2 rightPos = vec2( (texcoord.x-scorePosRight.x)/(rightWH.x), (texcoord.y-scorePosRight.y)/(rightWH.y) );
   scorecoordRight.x = score.y*0.1+rightPos.x*0.1;
   scorecoordRight.y = rightPos.y;
   //gl_Position = vec4(vertex, 0.0, 1.0);
   gl_Position = vec4(vertex*scale+offset, 0.0, 1.0);
}
