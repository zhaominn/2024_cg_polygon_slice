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
bool Line = true;
GLfloat pi = 3.14159265359f;
GLfloat gravity = 0.04f; // �߷�

struct GLPOINT {
	GLfloat x;
	GLfloat y;
};

struct IntersectionInfo {
	int polygonIndex; // ������ �������� �ε���
	int v1Index;      // ������ �������� ���� ù ��° ���� �ε���
	glm::vec3 intersection; // ���� ���� ��ǥ
};

struct Polygon {// �������� ������ ����ü
	int vertex_num;	// ������ ������ ����
	int start_index; // VAO������ ���� �ε���
	int move_num; // ������ Ƚ��
	float rotation_angle; //ȸ�� ����
	bool rotate;        // ȸ�� ����
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
void SetShape(int vertexNum, int startIndex, GLPOINT point, float angle) {
	float cos_theta = cos(angle);
	float sin_theta = sin(angle);

	switch (vertexNum) {
	case 3: // �ﰢ��
		vertexPosition[startIndex] = { point.x + 0.15f * cos(angle * pi / 180), point.y + 0.15f * sin(angle * pi / 180), 0.0f };
		vertexPosition[startIndex + 1] = { point.x + 0.15f * cos((angle + 120) * pi / 180), point.y + 0.15f * sin((angle + 120) * pi / 180), 0.0f };
		vertexPosition[startIndex + 2] = { point.x + 0.15f * cos((angle + 240) * pi / 180), point.y + 0.15f * sin((angle + 240) * pi / 180), 0.0f };
		break;
	case 4: // �簢��
		vertexPosition[startIndex] = { point.x + 0.15f * cos(angle * pi / 180), point.y + 0.15f * sin(angle * pi / 180), 0.0f };
		vertexPosition[startIndex + 1] = { point.x + 0.15f * cos((angle + 90) * pi / 180), point.y + 0.15f * sin((angle + 90) * pi / 180), 0.0f };
		vertexPosition[startIndex + 2] = { point.x + 0.15f * cos((angle + 270) * pi / 180), point.y + 0.15f * sin((angle + 270) * pi / 180), 0.0f };
		vertexPosition[startIndex + 3] = { point.x + 0.15f * cos((angle + 180) * pi / 180), point.y + 0.15f * sin((angle + 180) * pi / 180), 0.0f };
		break;
	case 5: // ������
		vertexPosition[startIndex] = { point.x + 0.15f * cos(angle * pi / 180), point.y + 0.15f * sin(angle * pi / 180), 0.0f };
		vertexPosition[startIndex + 1] = { point.x + 0.15f * cos((angle + 72) * pi / 180), point.y + 0.15f * sin((angle + 72) * pi / 180), 0.0f };
		vertexPosition[startIndex + 2] = { point.x + 0.15f * cos((angle + 288) * pi / 180), point.y + 0.15f * sin((angle + 288) * pi / 180), 0.0f };
		vertexPosition[startIndex + 3] = { point.x + 0.15f * cos((angle + 144) * pi / 180), point.y + 0.15f * sin((angle + 144) * pi / 180), 0.0f };
		vertexPosition[startIndex + 4] = { point.x + 0.15f * cos((angle + 216) * pi / 180), point.y + 0.15f * sin((angle + 216) * pi / 180), 0.0f };
		break;
	}
}

void FallShape(int vertexNum, int startIndex, GLPOINT point) {
	switch (vertexNum) {
	case 3: // �ﰢ��
		vertexPosition[startIndex].y -= gravity;
		vertexPosition[startIndex + 1].y -= gravity;
		vertexPosition[startIndex + 2].y -= gravity;
		break;
	case 4: // �簢��
		vertexPosition[startIndex].y -= gravity;
		vertexPosition[startIndex + 1].y -= gravity;
		vertexPosition[startIndex + 2].y -= gravity;
		vertexPosition[startIndex + 3].y -= gravity;
		break;
	case 5: // ������
		vertexPosition[startIndex].y -= gravity;
		vertexPosition[startIndex + 1].y -= gravity;
		vertexPosition[startIndex + 2].y -= gravity;
		vertexPosition[startIndex + 3].y -= gravity;
		vertexPosition[startIndex + 4].y -= gravity;
		break;
	}
}

// ������ ���� �Լ�
void SetPolygon() {
	struct::Polygon temp;
	int direction = (rand() % 2 == 0) ? -1 : 1;
	GLPOINT p1 = { direction * (-1.2f) ,(rand() % 10) / 10.0f - 0.5f };
	GLPOINT p2 = { direction * ((rand() % 18) / 10.0f - 0.9f),(rand() % 10) / 10.0f };
	GLPOINT p3 = { direction * ((rand() % 10) / 10.0f), -1.2f };
	temp = { rand() % 3 + 3, static_cast<int>(vertexPosition.size()), 0,0.0f,true, p1.x , p1.y, p1.x, p1.y, p2.x, p2.y,  p3.x, p3.y };

	GLfloat r = (rand() % 10) / 10.0f;
	GLfloat g = (rand() % 10) / 10.0f;
	GLfloat b = (rand() % 10) / 10.0f;
	for (int i = 0; i < temp.vertex_num; ++i) {
		vertexColor.push_back({ r,g,b });
		vertexPosition.push_back({ 0.0f,0.0f,0.0f });
	}
	polygons.push_back(temp);
	SetShape(temp.vertex_num, temp.start_index, temp.point, temp.rotation_angle);
}

// ���� ���� �Լ�: ���� A(x1, y1) -> B(x2, y2)�� ���� C(x3, y3) -> D(x4, y4) ���� ���� Ȯ��
bool LineSegmentsIntersect(glm::vec3 A, glm::vec3 B, glm::vec3 C, glm::vec3 D, glm::vec3& intersection) {
	auto cross = [](glm::vec3 v1, glm::vec3 v2) {
		return v1.x * v2.y - v1.y * v2.x;
		};

	glm::vec3 AB = B - A;
	glm::vec3 AC = C - A;
	glm::vec3 AD = D - A;
	glm::vec3 CD = D - C;
	glm::vec3 CA = A - C;
	glm::vec3 CB = B - C;

	float cross1 = cross(AB, AC);
	float cross2 = cross(AB, AD);
	float cross3 = cross(CD, CA);
	float cross4 = cross(CD, CB);

	if ((cross1 * cross2 < 0) && (cross3 * cross4 < 0)) {
		// ���� ���� ���
		float t = cross(CD, CA) / cross(CD, AB);
		intersection = A + t * AB;
		return true;
	}

	return false;
}

void SplitPolygon(const std::vector<IntersectionInfo>& intersections, const struct Polygon& polygon) {
	if (intersections.size() < 2) {
		std::cout << "���� ������ ������� �ʽ��ϴ�." << std::endl;
		return;
	}

	// �� ���� ������ ������ �����ɴϴ�.
	const glm::vec3& intersection1 = intersections[0].intersection;
	const glm::vec3& intersection2 = intersections[1].intersection;

	int startIndex = polygon.start_index;

	// �������� �����ϴ� �ﰢ�� ���� ���ʰ� ���������� ������ ���� ���
	for (int i = 0; i < polygon.vertex_num - 2; ++i) {
		glm::vec3 v1 = vertexPosition[startIndex];
		glm::vec3 v2 = vertexPosition[startIndex + i + 1];
		glm::vec3 v3 = vertexPosition[startIndex + i + 2];

		std::vector<glm::vec3> leftVertices;
		std::vector<glm::vec3> rightVertices;

		// �� �ﰢ���� �����鿡 ���� ���� ������ �������� ������
		auto classifyVertex = [&](const glm::vec3& vertex) {
			glm::vec3 dir = intersection2 - intersection1;
			glm::vec3 rel = vertex - intersection1;
			float crossProduct = dir.x * rel.y - dir.y * rel.x;  // ������ ������ �̿��� ����/������ �Ǵ�

			if (crossProduct > 0) {
				leftVertices.push_back(vertex);
			}
			else {
				rightVertices.push_back(vertex);
			}
			};

		classifyVertex(v1);
		classifyVertex(v2);
		classifyVertex(v3);

		// ���� ������ ���� �����￡ ��� �߰�
		leftVertices.push_back(intersection1);
		leftVertices.push_back(intersection2);
		rightVertices.push_back(intersection1);
		rightVertices.push_back(intersection2);

		// ��� ���
		std::cout << "�ﰢ�� " << i + 1 << "�� ���� ������ ������:" << std::endl;
		for (const auto& vertex : leftVertices) {
			std::cout << "(" << vertex.x << ", " << vertex.y << ")" << std::endl;
		}

		std::cout << "�ﰢ�� " << i + 1 << "�� ������ ������ ������:" << std::endl;
		for (const auto& vertex : rightVertices) {
			std::cout << "(" << vertex.x << ", " << vertex.y << ")" << std::endl;
		}
	}
}

// �������� �巡���� ���� �浹 ���� Ȯ�� �Լ�
bool IsLineIntersectingPolygons(std::vector<struct Polygon>& polygons, glm::vec3 lineStart, glm::vec3 lineEnd, std::vector<IntersectionInfo>& intersections) {
	glm::vec3 intersection;
	bool isIntersect = false;

	for (int i = 0; i < polygons.size(); ++i) {
		struct Polygon& polygon = polygons[i];
		for (int j = 0; j < polygon.vertex_num; ++j) {
			glm::vec3 v1 = vertexPosition[polygon.start_index + j];
			glm::vec3 v2 = vertexPosition[polygon.start_index + (j + 1) % polygon.vertex_num];

			if (LineSegmentsIntersect(lineStart, lineEnd, v1, v2, intersection)) {
				polygon.rotate = false; // ȸ�� ����
				intersections.push_back({ i, j, intersection });
				isIntersect = true;
			}
		}
	}
	return isIntersect;
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
	if (Line) {
		for (int i = 0; i < polygons.size(); ++i) {
			for (int j = 0; j < polygons[i].vertex_num - 2; ++j)
				glDrawArrays(GL_LINE_LOOP, currentIndex + j, 3);
			currentIndex += polygons[i].vertex_num;
		}
	}
	else {
		for (int i = 0; i < polygons.size(); ++i) {
			for (int j = 0; j < polygons[i].vertex_num - 2; ++j)
				glDrawArrays(GL_TRIANGLES, currentIndex + j, 3);
			currentIndex += polygons[i].vertex_num;
		}
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
		// ���콺 �巡�� ����
		GLfloat X, Y;
		ScreenToOpenGL(x, y, X, Y); // ��ǥ ��ȯ
		isDrag = true;
		vertexPosition[0] = { X, Y, 0.0f };
		vertexPosition[1] = { X, Y, 0.0f };
	}
	else if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) {
		isDrag = false;

		// ���� ���� Ȯ�� �� ���
		std::vector<IntersectionInfo> intersections;
		if (IsLineIntersectingPolygons(polygons, vertexPosition[0], vertexPosition[1], intersections)) {
			if (intersections.size() >= 2) {
				// ù ��° ������ ������ �������� ���ʰ� ������ ������ �������� ���
				SplitPolygon(intersections, polygons[intersections[0].polygonIndex]);
			}
			else {
				std::cout << "�� �� �̻��� ���� ������ �ʿ��մϴ�." << std::endl;
			}
		}
	}
	InitBuffer();
	glutPostRedisplay();
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

	bool needResetIndex = false;

	for (int i = 0; i < polygons.size(); ++i) {
		if (polygons[i].rotate) { // ȸ�� ���� Ȯ��
			polygons[i].rotation_angle += 3.0f; // ���� ����
			GLfloat t = polygons[i].move_num / 100.0f;
			polygons[i].point = {
				 (2 * t * t - 3 * t + 1) * polygons[i].p1.x + (-4 * t * t + 4 * t) * polygons[i].p2.x + (2 * t * t - t) * polygons[i].p3.x,
				  (2 * t * t - 3 * t + 1) * polygons[i].p1.y + (-4 * t * t + 4 * t) * polygons[i].p2.y + (2 * t * t - t) * polygons[i].p3.y
			};

			polygons[i].rotation_angle += 3.0f; // ���� ����
			SetShape(polygons[i].vertex_num, polygons[i].start_index, polygons[i].point, polygons[i].rotation_angle);
		}
		else {
			FallShape(polygons[i].vertex_num, polygons[i].start_index, polygons[i].point);
		}
		++polygons[i].move_num;
	}

	for (int i = 0; i < polygons.size(); ++i) {
		if (polygons[i].move_num >= 100) {
			int startIdx = polygons[i].start_index;
			int numVertices = polygons[i].vertex_num;

			vertexPosition.erase(vertexPosition.begin() + startIdx, vertexPosition.begin() + startIdx + numVertices);
			vertexColor.erase(vertexColor.begin() + startIdx, vertexColor.begin() + startIdx + numVertices);

			polygons.erase(polygons.begin() + i);
			--i;
			needResetIndex = true;
		}
	}

	if (needResetIndex) {
		int currentIndex = 2;
		for (int i = 0; i < polygons.size(); ++i) {
			polygons[i].start_index = currentIndex;
			currentIndex += polygons[i].vertex_num;
		}
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