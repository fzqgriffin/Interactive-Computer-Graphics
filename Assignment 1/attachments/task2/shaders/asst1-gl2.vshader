uniform float uVertexScale1;
uniform float uVertexScale2;
uniform float uTime;

attribute vec2 aPosition;
attribute vec3 aColor;
attribute vec2 aTexCoord0, aTexCoord1;

varying vec3 vColor;
varying vec2 vTexCoord0, vTexCoord1;

void main() {
  gl_Position = vec4(aPosition.x * uVertexScale1, aPosition.y * uVertexScale2, 0,1);
  //gl_Position = vec4(uVertexScale1, uVertexScale2, 0,1);
  vColor = aColor;
  vTexCoord0 = aTexCoord0;
  vTexCoord1 = aTexCoord1;
}
