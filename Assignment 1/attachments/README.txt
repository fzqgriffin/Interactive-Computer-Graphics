All codes can be supported on OS X 10.9.4 and OpenGL 2.x with GLSL 1.0.

Codes in Folder task2 are written for homework?s task 2.

Main changes in file asst1.cpp:
	At line 65 to 66, initiate two static variables as the scale factor for coordinates x and y of the vertex.
	At line 277 to 284, calculate the value of two scale factors according to the value of window?s width and height.

Main changes in file gl2.vshader:
	At line 13, change vec4 by adding uVertexScale2 (equals variable g_objScale2 at line 66 in asst1.cpp).



All codes can be supported on OS X 10.9.4 and OpenGL 2.x with GLSL 1.0.

Codes in Folder task3 are written for homework?s task 3.

Main changes in file asst1.cpp:
	At line 68, initiate a variable called g_visible to control the visibility of two pictures.
	At line 252 to 265, turn g_time into integers between 0 and 180, and use function sin() and g_time to ensure the value of g_visible will change from 0 to 1 and from 1 to 0.

Main changes in file gl2.fshader
	At line 13, use uVisible to replace .5*uVertexScale.
	Alt line 14, change .5*uVertexScale to 1.5.
