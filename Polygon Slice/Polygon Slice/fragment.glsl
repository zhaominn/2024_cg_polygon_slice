#version 330 core

in vec3 passColor; //--- vertex shader���� �Է� ����
out vec4 FragColor; //--- ������ ���۷� ���
void main()
{
FragColor = vec4 (passColor, 1.0);
}