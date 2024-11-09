#version 330 core
in vec3 vPos; //--- 메인 프로그램에서 입력 받음
in vec3 vColor; //--- 메인 프로그램에서 입력 받음
out vec3 passColor; //--- fragment shader로 전달
void main()
{
gl_Position = vec4 (vPos.x, vPos.y, vPos.z, 1.0);
passColor = vColor;
}