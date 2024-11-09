#define _CRT_SECURE_NO_WARNINGS //--- 프로그램 맨 앞에 선언할 것
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
	//--- 버텍스 세이더 읽어 저장하고 컴파일 하기
	//--- filetobuf: 사용자정의 함수로 텍스트를 읽어서 문자열에 저장하는 함수
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
	//--- 프래그먼트 세이더 읽어 저장하고 컴파일하기
	fragmentSource = filetobuf("fragment.glsl"); // 프래그세이더 읽어오기
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
	shaderID = glCreateProgram(); //--- 세이더 프로그램 만들기

	glAttachShader(shaderID, vertexShader); //--- 세이더 프로그램에 버텍스 세이더 붙이기
	glAttachShader(shaderID, fragmentShader); //--- 세이더 프로그램에 프래그먼트 세이더 붙이기

	glLinkProgram(shaderID); //--- 세이더 프로그램 링크하기
	glDeleteShader(vertexShader); //--- 세이더 객체를 세이더 프로그램에 링크했음으로, 세이더 객체 자체는 삭제 가능

	glDeleteShader(fragmentShader);

	glGetProgramiv(shaderID, GL_LINK_STATUS, &result); // ---세이더가 잘 연결되었는지 체크하기
	if (!result) {
		glGetProgramInfoLog(shaderID, 512, NULL, errorLog);
		std::cerr << "ERROR: shader program 연결 실패\n" << errorLog << std::endl;
		return false;
	}

	glUseProgram(shaderID); //--- 만들어진 세이더 프로그램 사용하기
	return shaderID;
}
//-----------------------------------------------------------------------------
bool isDrag = false;

struct GLPOINT {
	GLfloat x;
	GLfloat y;
};

struct Polygon {// 도형들을 저장할 구조체
	int vertex_num;	// 도형의 정점의 개수
	int start_index; // VAO에서의 시작 인덱스
	int move_num; // 움직임 횟수
	GLPOINT point; // 폴리곤 위치
	GLPOINT p1; // 곡선 이동 경로 설정을 위한 정점
	GLPOINT p2;
	GLPOINT p3;
};

int TimeNum = 0;

std::vector<struct::Polygon> polygons;

std::vector<glm::vec3> vertexColor = {
	//마우스 드래그 선
	glm::vec3(0.0f, 0.0f, 0.0f),
	glm::vec3(0.0f, 0.0f, 0.0f),
};
std::vector<glm::vec3> vertexPosition = {
	//마우스 드래그 선
	glm::vec3(1.0f,0.0f,0.0f),
	glm::vec3(-1.0f,0.0f,0.0f),
};
GLuint VAO, VBO_position, VBO_color;

void InitBuffer()
{
	//--- Vertex Array Object 생성
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	//--- 위치 속성
	glGenBuffers(1, &VBO_position);
	glBindBuffer(GL_ARRAY_BUFFER, VBO_position);
	glBufferData(GL_ARRAY_BUFFER, vertexPosition.size() * sizeof(glm::vec3), glm::value_ptr(vertexPosition[0]), GL_STATIC_DRAW);

	//--- 색상 속성
	glGenBuffers(1, &VBO_color);
	glBindBuffer(GL_ARRAY_BUFFER, VBO_color);
	glBufferData(GL_ARRAY_BUFFER, vertexColor.size() * sizeof(glm::vec3), glm::value_ptr(vertexColor[0]), GL_STATIC_DRAW);

	//--- vPos 속성 변수에 값을 저장
	GLint pAttribute = glGetAttribLocation(shaderProgramID, "vPos");
	glBindBuffer(GL_ARRAY_BUFFER, VBO_position);
	glVertexAttribPointer(pAttribute, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
	glEnableVertexAttribArray(pAttribute);

	//--- vColor 속성 변수에 값을 저장
	GLint cAttribute = glGetAttribLocation(shaderProgramID, "vColor");
	glBindBuffer(GL_ARRAY_BUFFER, VBO_color);
	glVertexAttribPointer(cAttribute, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
	glEnableVertexAttribArray(cAttribute);
}

// 스크린 좌표를 OpenGL 좌표로 변환하는 함수
void ScreenToOpenGL(int x, int y, GLfloat& X, GLfloat& Y) {
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	int windowWidth = viewport[2];
	int windowHeight = viewport[3];

	X = (static_cast<float>(x) / static_cast<float>(windowWidth)) * 2.0f - 1.0f;
	Y = 1.0f - (static_cast<float>(y) / static_cast<float>(windowHeight)) * 2.0f;
}

// 중심 점을 이용해 모양 별 버텍스 설정하기
void SetShape(int vertexNum, int startIndex, GLPOINT point) {
	switch (vertexNum) {
	case 3:
		vertexPosition[startIndex] = { point.x, point.y + 0.15f, 0.0f };
		vertexPosition[startIndex + 1] = { point.x - 0.12f, point.y - 0.09f, 0.0f };
		vertexPosition[startIndex + 2] = { point.x + 0.12f, point.y - 0.09f, 0.0f };
		break;
	case 4:
		vertexPosition[startIndex] = { point.x - 0.12f, point.y + 0.12f, 0.0f };
		vertexPosition[startIndex + 1] = { point.x + 0.12f, point.y + 0.12f, 0.0f };
		vertexPosition[startIndex + 2] = { point.x - 0.12f, point.y - 0.12f, 0.0f };
		vertexPosition[startIndex + 3] = { point.x + 0.12f, point.y - 0.12f, 0.0f };
		break;
	}
}

// 폴리곤 설정 함수
void SetPolygon() {
	struct::Polygon temp;
	int direction = (rand() % 2 == 0) ? -1 : 1;
	GLPOINT p1 = { direction * (-1.2f) ,(rand() % 10) / 10.0f - 0.5f };
	GLPOINT p2 = { direction * ((rand() % 18) / 10.0f - 0.9f),(rand() % 10) / 10.0f };
	GLPOINT p3 = { direction * ((rand() % 10) / 10.0f), -1.2f };
	temp = { rand() % 2 + 3, static_cast<int>(vertexPosition.size()), 0, p1.x , p1.y, p1.x, p1.y, p2.x, p2.y,  p3.x, p3.y };
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

	int currentIndex = 2;
	for (int i = 0; i < polygons.size(); ++i) {
		for (int j = 0; j < polygons[i].vertex_num - 2; ++j)
			glDrawArrays(GL_TRIANGLES, currentIndex + j, 3);

		currentIndex += polygons[i].vertex_num;
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
		//마우스 드래그 시작 선
		GLfloat X, Y;
		ScreenToOpenGL(x, y, X, Y); // 좌표 변환
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
	//마우스가움직이는중일때
	if (isDrag) {
		GLfloat X, Y;
		ScreenToOpenGL(x, y, X, Y); // 좌표 변환

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

	bool needResetIndex = false;

	for (int i = 0; i < polygons.size(); ++i) {
		GLfloat t = polygons[i].move_num / 100.0f;
		polygons[i].point = {
			 (2 * t * t - 3 * t + 1) * polygons[i].p1.x + (-4 * t * t + 4 * t) * polygons[i].p2.x + (2 * t * t - t) * polygons[i].p3.x,
			  (2 * t * t - 3 * t + 1) * polygons[i].p1.y + (-4 * t * t + 4 * t) * polygons[i].p2.y + (2 * t * t - t) * polygons[i].p3.y
		};

		SetShape(polygons[i].vertex_num, polygons[i].start_index, polygons[i].point);
		++polygons[i].move_num;
	}

	for (int i = 0; i < polygons.size(); ++i) {
		if (polygons[i].move_num >= 100) {
			int startIdx = polygons[i].start_index;
			int numVertices = polygons[i].vertex_num;

			// vertexPosition과 vertexColor에서 정점 데이터 삭제
			vertexPosition.erase(vertexPosition.begin() + startIdx, vertexPosition.begin() + startIdx + numVertices);
			vertexColor.erase(vertexColor.begin() + startIdx, vertexColor.begin() + startIdx + numVertices);

			polygons.erase(polygons.begin() + i);
			--i; // 인덱스 조정
			needResetIndex = true; // 재설정이 필요함을 표시
		}
	}

	if (needResetIndex) {
		int currentIndex = 2; // 초기 시작 인덱스
		for (int i = 0; i < polygons.size(); ++i) {
			polygons[i].start_index = currentIndex;
			currentIndex += polygons[i].vertex_num; // 각 폴리곤의 정점 개수만큼 인덱스를 증가시킴
		}
	}

	InitBuffer();
	glutPostRedisplay();
	glutTimerFunc(60, TimerFunction, 1);
}

void main(int argc, char** argv) //--- 윈도우 출력하고 콜백함수 설정
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