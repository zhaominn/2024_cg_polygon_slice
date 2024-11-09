#version 330 core

in vec3 passColor; //--- vertex shader에서 입력 받음
out vec4 FragColor; //--- 프레임 버퍼로 출력
void main()
{
FragColor = vec4 (passColor, 1.0);
}