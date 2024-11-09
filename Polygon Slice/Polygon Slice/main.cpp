#define _CRT_SECURE_NO_WARNINGS //--- ���α׷� �� �տ� ������ ��
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <gl/glew.h>
#include<gl/freeglut.h>
#include <gl/freeglut_ext.h>
#include <glm/glm/glm.hpp>
#include <glm/glm/ext.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>
#include <vector>
void make_vertexShaders();
void make_fragmentShaders();
GLuint make_shaderProgram();
GLuint shaderProgramID;
GLuint vertexShader;
GLuint fragmentShader;
char* filetobuf(const char* file)
{
	FILE* fptr;
	long length;
	char* buf;
	fptr = fopen(file, "rb");
	if (!fptr)
		return NULL;
	fseek(fptr, 0, SEEK_END);
	length = ftell(fptr);
	buf = (char*)malloc(length + 1);
	fseek(fptr, 0, SEEK_SET);
	fread(buf, length, 1, fptr);
	fclose(fptr);
	buf[length] = 0;
	return buf;
}
void make_vertexShaders()
{
	GLchar* vertexSource;
	//--- ���ؽ� ���̴� �о� �����ϰ� ������ �ϱ�
	//--- filetobuf: ��������� �Լ��� �ؽ�Ʈ�� �о ���ڿ��� �����ϴ� �Լ�
	vertexSource = filetobuf("vertex.glsl");
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexSource, NULL);
	glCompileShader(vertexShader);
	GLint result;
	GLchar errorLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &result);
}
void make_fragmentShaders()
{
	GLchar* fragmentSource;
	//--- �����׸�Ʈ ���̴� �о� �����ϰ� �������ϱ�
	fragmentSource = filetobuf("fragment.glsl"); // �����׼��̴� �о����
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
	glCompileShader(fragmentShader);
	GLint result;
	GLchar errorLog[512];
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &result);
}
GLuint make_shaderProgram()
{
	GLchar errorLog[512];
	GLint result;

	GLuint shaderID;
	shaderID = glCreateProgram(); //--- ���̴� ���α׷� �����

	glAttachShader(shaderID, vertexShader); //--- ���̴� ���α׷��� ���ؽ� ���̴� ���̱�
	glAttachShader(shaderID, fragmentShader); //--- ���̴� ���α׷��� �����׸�Ʈ ���̴� ���̱�

	glLinkProgram(shaderID); //--- ���̴� ���α׷� ��ũ�ϱ�
	glDeleteShader(vertexShader); //--- ���̴� ��ü�� ���̴� ���α׷��� ��ũ��������, ���̴� ��ü ��ü�� ���� ����

	glDeleteShader(fragmentShader);

	glGetProgramiv(shaderID, GL_LINK_STATUS, &result); // ---���̴��� �� ����Ǿ����� üũ�ϱ�
	if (!result) {
		glGetProgramInfoLog(shaderID, 512, NULL, errorLog);
		std::cerr << "ERROR: shader program ���� ����\n" << errorLog << std::endl;
		return false;
	}

	glUseProgram(shaderID); //--- ������� ���̴� ���α׷� ����ϱ�
	return shaderID;
}
//-----------------------------------------------------------------------------
bool isDrag = false;

struct GLPOINT {
	GLfloat x;
	GLfloat y;
};

struct Polygon {// �������� ������ ����ü
	int vertex_num;	// ������ ������ ����
	int start_index; // VAO������ ���� �ε���
	int move_num; // ������ Ƚ��
	GLPOINT point; // ������ ��ġ
	GLPOINT p1; // � �̵� ��� ������ ���� ����
	GLPOINT p2;
	GLPOINT p3;
};

int TimeNum = 0;

std::vector<struct::Polygon> polygons;

std::vector<glm::vec3> vertexColor = {
	//���콺 �巡�� ��
	glm::vec3(0.0f, 0.0f, 0.0f),
	glm::vec3(0.0f, 0.0f, 0.0f),
};
std::vector<glm::vec3> vertexPosition = {
	//���콺 �巡�� ��
	glm::vec3(1.0f,0.0f,0.0f),
	glm::vec3(-1.0f,0.0f,0.0f),
};
GLuint VAO, VBO_position, VBO_color;

void InitBuffer()
{
	//--- Vertex Array Object ����
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	//--- ��ġ �Ӽ�
	glGenBuffers(1, &VBO_position);
	glBindBuffer(GL_ARRAY_BUFFER, VBO_position);
	glBufferData(GL_ARRAY_BUFFER, vertexPosition.size() * sizeof(glm::vec3), glm::value_ptr(vertexPosition[0]), GL_STATIC_DRAW);

	//--- ���� �Ӽ�
	glGenBuffers(1, &VBO_color);
	glBindBuffer(GL_ARRAY_BUFFER, VBO_color);
	glBufferData(GL_ARRAY_BUFFER, vertexColor.size() * sizeof(glm::vec3), glm::value_ptr(vertexColor[0]), GL_STATIC_DRAW);

	//--- vPos �Ӽ� ������ ���� ����
	GLint pAttribute = glGetAttribLocation(shaderProgramID, "vPos");
	glBindBuffer(GL_ARRAY_BUFFER, VBO_position);
	glVertexAttribPointer(pAttribute, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
	glEnableVertexAttribArray(pAttribute);

	//--- vColor �Ӽ� ������ ���� ����
	GLint cAttribute = glGetAttribLocation(shaderProgramID, "vColor");
	glBindBuffer(GL_ARRAY_BUFFER, VBO_color);
	glVertexAttribPointer(cAttribute, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
	glEnableVertexAttribArray(cAttribute);
}

// ��ũ�� ��ǥ�� OpenGL ��ǥ�� ��ȯ�ϴ� �Լ�
void ScreenToOpenGL(int x, int y, GLfloat& X, GLfloat& Y) {
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	int windowWidth = viewport[2];
	int windowHeight = viewport[3];

	X = (static_cast<float>(x) / static_cast<float>(windowWidth)) * 2.0f - 1.0f;
	Y = 1.0f - (static_cast<float>(y) / static_cast<float>(windowHeight)) * 2.0f;
}

// �߽� ���� �̿��� ��� �� ���ؽ� �����ϱ�
void SetShape(int vertexNum, int startIndex, GLPOINT point) {
	switch (vertexNum) {
	case 3:
		vertexPosition[startIndex] = { point.x, point.y + 0.1f, 0.0f };
		vertexPosition[startIndex + 1] = { point.x - 0.08f, point.y - 0.06f, 0.0f };
		vertexPosition[startIndex + 2] = { point.x + 0.08f, point.y - 0.06f, 0.0f };
		break;
	}
}

// ������ ���� �Լ�
void SetPolygon() {
	struct::Polygon temp;
	temp = { 3, 2 + 3*static_cast<int>(polygons.size()), 0, -1.0f, -0.2f, -1.0f, -0.2f, 0.0f, 0.0f, 1.0f, -1.0f };
	//temp.vertex_num = rand() % 3 + 3;
	GLfloat r = (rand() % 10) / 10.0f;
	GLfloat g = (rand() % 10) / 10.0f;
	GLfloat b = (rand() % 10) / 10.0f;
	for (int i = 0; i < temp.vertex_num; ++i) {
		vertexColor.push_back({ r,g,b });
		vertexPosition.push_back({ 0.0f,0.0f,0.0f });
	}

	polygons.push_back(temp);
	SetShape(temp.vertex_num, temp.start_index, temp.point);
}

GLvoid drawScene()
{
	glClearColor(1.0f, 0.8f, 0.8f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(shaderProgramID);
	glBindVertexArray(VAO);

	if (isDrag) {
		glDrawArrays(GL_LINE_STRIP, 0, 2);
	}

	for (int i = 0; i < polygons.size(); ++i) {
		glDrawArrays(GL_TRIANGLES, 2 + 3 * i, 3);
	}

	glutSwapBuffers();
}

GLvoid Keyboard(unsigned char key, int x, int y)
{
	switch (key) {
	}
	InitBuffer();
	glutPostRedisplay();
}

void Mouse(int button, int state, int x, int y) {
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
		//���콺 �巡�� ���� ��
		GLfloat X, Y;
		ScreenToOpenGL(x, y, X, Y); // ��ǥ ��ȯ
		isDrag = true;
		vertexPosition[0] = { X,Y,0.0f };
		vertexPosition[1] = { X,Y,0.0f };
	}
	else if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) {

		isDrag = false;
	}
	InitBuffer();
	glutPostRedisplay();
}

void Motion(int x, int y) {
	//���콺�������̴����϶�
	if (isDrag) {
		GLfloat X, Y;
		ScreenToOpenGL(x, y, X, Y); // ��ǥ ��ȯ

		vertexPosition[1] = { X,Y,0.0f };
	}

	glutPostRedisplay();
}

void TimerFunction(int value)
{
	++TimeNum;
	if (TimeNum >= 30) {
		SetPolygon();
		TimeNum = 0;
	}
	
	for (int i = 0; i < polygons.size(); ++i) {
		GLfloat t = polygons[i].move_num / 100.0f;
		polygons[i].point = {
			 (2 * t * t - 3 * t + 1) * polygons[i].p1.x + (-4 * t * t + 4 * t) * polygons[i].p2.x + (2 * t * t - t) * polygons[i].p3.x,
			  (2 * t * t - 3 * t + 1) * polygons[i].p1.y + (-4 * t * t + 4 * t) * polygons[i].p2.y + (2 * t * t - t) * polygons[i].p3.y
		};

		SetShape(polygons[i].vertex_num, polygons[i].start_index, polygons[i].point);
		++polygons[i].move_num;
	}

	InitBuffer();
	glutPostRedisplay();
	glutTimerFunc(60, TimerFunction, 1);
}

void main(int argc, char** argv) //--- ������ ����ϰ� �ݹ��Լ� ����
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(800, 800);
	glutCreateWindow("Slice Game");

	glewExperimental = GL_TRUE;
	glewInit();

	make_vertexShaders();
	make_fragmentShaders();
	shaderProgramID = make_shaderProgram();

	InitBuffer();

	glutDisplayFunc(drawScene);
	glutKeyboardFunc(Keyboard);
	glutTimerFunc(60, TimerFunction, 1);
	glutMouseFunc(Mouse);
	glutMotionFunc(Motion);
	glutMainLoop();

}