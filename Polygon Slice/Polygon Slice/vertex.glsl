#version 330 core
in vec3 vPos; //--- ���� ���α׷����� �Է� ����
in vec3 vColor; //--- ���� ���α׷����� �Է� ����
out vec3 passColor; //--- fragment shader�� ����
void main()
{
gl_Position = vec4 (vPos.x, vPos.y, vPos.z, 1.0);
passColor = vColor;
}