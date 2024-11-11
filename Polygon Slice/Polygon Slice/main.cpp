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
#include <algorithm>
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
	fragmentSource = filetobuf("fragment.glsl");
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
	shaderID = glCreateProgram(); 

	glAttachShader(shaderID, vertexShader); 
	glAttachShader(shaderID, fragmentShader); 

	glLinkProgram(shaderID); 
	glDeleteShader(vertexShader); 

	glDeleteShader(fragmentShader);

	glGetProgramiv(shaderID, GL_LINK_STATUS, &result); 
	if (!result) {
		glGetProgramInfoLog(shaderID, 512, NULL, errorLog);
		std::cerr << "ERROR: shader program 연결 실패\n" << errorLog << std::endl;
		return false;
	}

	glUseProgram(shaderID);
	return shaderID;
}
//-----------------------------------------------------------------------------
bool isDrag = false;
bool Line = false;
bool DrawPath = false;
GLfloat Speed = 60;
GLfloat pi = 3.14159265359f;
GLfloat gravity = 0.05f; // 중력
std::vector<int> DeletePolygonIndexList; // 삭제할 폴리곤의 인덱스 저장하는 벡터

struct GLPOINT {
	GLfloat x;
	GLfloat y;
};
struct IntersectionInfo {
	int polygonIndex; // 교차된 폴리곤의 인덱스
	int v1Index;      // 교차된 폴리곤의 변의 첫 번째 정점 인덱스
	glm::vec3 intersection; // 교차 지점 좌표
};
struct Polygon {// 도형들을 저장할 구조체
	int vertex_num;	// 도형의 정점의 개수
	int start_index; // VAO에서의 시작 인덱스
	int move_num; // 움직임 횟수
	float rotation_angle; //회전 각도
	int rotate;        // 회전 여부
	GLPOINT point; // 폴리곤 위치
	GLPOINT p1; // 곡선 이동 경로 설정을 위한 정점
	GLPOINT p2;
	GLPOINT p3;
	GLfloat dx;//회전하지 않을 때 x값 변화량
};
int TimeNum = 0;
std::vector<struct::Polygon> polygons;

GLfloat bucket_dx = 0.01f;

std::vector<glm::vec3> vertexColor = {
	// 마우스 드래그 선
	glm::vec3(1.0f, 0.0f, 0.0f),
	glm::vec3(1.0f, 0.0f, 0.0f),
	// 바구니
	glm::vec3(1.0f, 0.6f, 0.6f),
	glm::vec3(1.0f, 0.6f, 0.6f),
	glm::vec3(1.0f, 0.6f, 0.6f),
	glm::vec3(1.0f, 0.6f, 0.6f),
};
std::vector<glm::vec3> vertexPosition = {
	// 마우스 드래그 선
	glm::vec3(1.0f,0.0f,0.0f),
	glm::vec3(-1.0f,0.0f,0.0f),
	// 바구니
	glm::vec3(-0.25f, -0.85f, 0.0f),
	glm::vec3(0.25f, -0.85f, 0.0f),
	glm::vec3(-0.25f, -0.9f, 0.0f),
	glm::vec3(0.25f, -0.9f, 0.0f),
};
GLuint VAO, VBO_position, VBO_color;

void InitBuffer()
{
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	glGenBuffers(1, &VBO_position);
	glBindBuffer(GL_ARRAY_BUFFER, VBO_position);
	glBufferData(GL_ARRAY_BUFFER, vertexPosition.size() * sizeof(glm::vec3), glm::value_ptr(vertexPosition[0]), GL_STATIC_DRAW);

	glGenBuffers(1, &VBO_color);
	glBindBuffer(GL_ARRAY_BUFFER, VBO_color);
	glBufferData(GL_ARRAY_BUFFER, vertexColor.size() * sizeof(glm::vec3), glm::value_ptr(vertexColor[0]), GL_STATIC_DRAW);

	GLint pAttribute = glGetAttribLocation(shaderProgramID, "vPos");
	glBindBuffer(GL_ARRAY_BUFFER, VBO_position);
	glVertexAttribPointer(pAttribute, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
	glEnableVertexAttribArray(pAttribute);

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

// 회전할 경우 중심 점을 이용해 버텍스 설정
void SetShape(int vertexNum, int startIndex, GLPOINT point, float angle) {
	float cos_theta = cos(angle);
	float sin_theta = sin(angle);

	switch (vertexNum) {
	case 3: // 삼각형
		vertexPosition[startIndex] = { point.x + 0.15f * cos(angle * pi / 180), point.y + 0.15f * sin(angle * pi / 180), 0.0f };
		vertexPosition[startIndex + 1] = { point.x + 0.15f * cos((angle + 120) * pi / 180), point.y + 0.15f * sin((angle + 120) * pi / 180), 0.0f };
		vertexPosition[startIndex + 2] = { point.x + 0.15f * cos((angle + 240) * pi / 180), point.y + 0.15f * sin((angle + 240) * pi / 180), 0.0f };
		break;
	case 4: // 사각형
		vertexPosition[startIndex] = { point.x + 0.15f * cos(angle * pi / 180), point.y + 0.15f * sin(angle * pi / 180), 0.0f };
		vertexPosition[startIndex + 1] = { point.x + 0.15f * cos((angle + 90) * pi / 180), point.y + 0.15f * sin((angle + 90) * pi / 180), 0.0f };
		vertexPosition[startIndex + 2] = { point.x + 0.15f * cos((angle + 270) * pi / 180), point.y + 0.15f * sin((angle + 270) * pi / 180), 0.0f };
		vertexPosition[startIndex + 3] = { point.x + 0.15f * cos((angle + 180) * pi / 180), point.y + 0.15f * sin((angle + 180) * pi / 180), 0.0f };
		break;
	case 5: // 오각형
		vertexPosition[startIndex] = { point.x + 0.15f * cos(angle * pi / 180), point.y + 0.15f * sin(angle * pi / 180), 0.0f };
		vertexPosition[startIndex + 1] = { point.x + 0.15f * cos((angle + 72) * pi / 180), point.y + 0.15f * sin((angle + 72) * pi / 180), 0.0f };
		vertexPosition[startIndex + 2] = { point.x + 0.15f * cos((angle + 288) * pi / 180), point.y + 0.15f * sin((angle + 288) * pi / 180), 0.0f };
		vertexPosition[startIndex + 3] = { point.x + 0.15f * cos((angle + 144) * pi / 180), point.y + 0.15f * sin((angle + 144) * pi / 180), 0.0f };
		vertexPosition[startIndex + 4] = { point.x + 0.15f * cos((angle + 216) * pi / 180), point.y + 0.15f * sin((angle + 216) * pi / 180), 0.0f };
		break;
	}
}

void FallShape(int vertexNum, int startIndex, GLPOINT point, int PolygonIndex) {
	for (int i = 0; i < vertexNum; ++i) {
		vertexPosition[startIndex + i].x -= polygons[PolygonIndex].dx;
		vertexPosition[startIndex + i].y -= gravity;
	}
}

// 폴리곤 설정 함수
void SetPolygon() {
	struct::Polygon temp;
	int direction = (rand() % 2 == 0) ? -1 : 1;
	GLPOINT p1 = { direction * (-1.2f) ,(rand() % 10) / 10.0f - 0.5f };
	GLPOINT p2 = { direction * ((rand() % 18) / 10.0f - 0.9f),(rand() % 10) / 10.0f };
	GLPOINT p3 = { -direction * (-1.2f) ,(rand() % 10) / 10.0f - 1.0f };
	temp = { rand() % 3 + 3, static_cast<int>(vertexPosition.size()), 0,0.0f,true, p1, p1, p2,  p3 , (rand() % 80 - 40) / 1000.0f };

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

// 선분 A -> B와 선분 C -> D 교차 여부 확인
bool LineSegmentsIntersect(glm::vec3 A, glm::vec3 B, glm::vec3 C, glm::vec3 D, glm::vec3& intersection) {
	glm::vec3 AB = B - A;
	glm::vec3 CD = D - C;

	float det = AB.x * CD.y - AB.y * CD.x;

	if (std::abs(det) < 1e-6) {
		return false;
	}

	glm::vec3 AC = C - A;

	float t = (AC.x * CD.y - AC.y * CD.x) / det;
	float u = (AC.x * AB.y - AC.y * AB.x) / det;

	if (t >= 0.0f && t <= 1.0f && u >= 0.0f && u <= 1.0f) {
		intersection = A + t * AB;
		return true;
	}

	return false;
}

// 마우스 드래그 선분과 교차된 폴리곤을 나누어 폴리곤 버텍스에 새로 추가하는 함수^^
void AddNewPolygon(const std::vector<glm::vec3>& vertices, glm::vec3 originalColor, struct Polygon& polygon, GLfloat dx) {
	int vertexCount = vertices.size();
	if (vertexCount < 3) return; // 삼각형 이상인 경우에만 추가

	struct::Polygon newPolygon = { vertexCount, static_cast<int>(vertexPosition.size()), 0, 0.0f, false, polygon.point, polygon.p1, polygon.p2, polygon.p3, dx };
	polygons.push_back(newPolygon);

	for (int j = 0; j < vertexCount; ++j) {
		vertexColor.push_back(originalColor);
	}

	switch (vertices.size()) {
	case 3:
		vertexPosition.push_back(vertices[0]);
		vertexPosition.push_back(vertices[1]);
		vertexPosition.push_back(vertices[2]);
		break;
	case 4:
		vertexPosition.push_back(vertices[0]);
		vertexPosition.push_back(vertices[1]);
		vertexPosition.push_back(vertices[3]);
		vertexPosition.push_back(vertices[2]);
		break;
	case 5:
		vertexPosition.push_back(vertices[0]);
		vertexPosition.push_back(vertices[1]);
		vertexPosition.push_back(vertices[4]);
		vertexPosition.push_back(vertices[2]);
		vertexPosition.push_back(vertices[3]);
		break;
	case 6:
		vertexPosition.push_back(vertices[0]);
		vertexPosition.push_back(vertices[1]);
		vertexPosition.push_back(vertices[5]);
		vertexPosition.push_back(vertices[2]);
		vertexPosition.push_back(vertices[4]);
		vertexPosition.push_back(vertices[3]);
		break;
	}
}

// 마우스 드래그 선분을 기준으로 정점을 나누고 정렬하기
void SplitPolygon(const std::vector<IntersectionInfo>& intersections, struct Polygon& polygon, int polygonNum) {
	if (intersections.size() < 2) {
		return; 
	}

	int startIndex = polygon.start_index;
	glm::vec3 originalColor = vertexColor[startIndex];

	glm::vec3 intersection1 = intersections[0].intersection;
	glm::vec3 intersection2 = intersections[1].intersection;

	std::vector<glm::vec3> leftVertices;
	std::vector<glm::vec3> rightVertices;

	for (int i = 0; i < polygon.vertex_num; ++i) {
		glm::vec3 vertex = vertexPosition[startIndex + i];
		glm::vec3 dir = intersection2 - intersection1;
		glm::vec3 rel = vertex - intersection1;

		float crossProduct = dir.x * rel.y - dir.y * rel.x;

		if (crossProduct > 0) {
			leftVertices.push_back(vertex);
		}
		else {
			rightVertices.push_back(vertex);
		}
	}

	auto addIfNotPresent = [](std::vector<glm::vec3>& vertices, const glm::vec3& point) {
		if (std::find(vertices.begin(), vertices.end(), point) == vertices.end()) {
			vertices.push_back(point);
		}
		};
	addIfNotPresent(leftVertices, intersection1);
	addIfNotPresent(leftVertices, intersection2);
	addIfNotPresent(rightVertices, intersection1);
	addIfNotPresent(rightVertices, intersection2);

	auto sortVertices = [](std::vector<glm::vec3>& vertices) {
		if (vertices.size() < 3) return;

		glm::vec3 centroid(0.0f, 0.0f, 0.0f);
		for (const auto& vertex : vertices) {
			centroid += vertex;
		}
		centroid /= static_cast<float>(vertices.size());

		auto angleComparator = [&centroid](const glm::vec3& a, const glm::vec3& b) {
			float angleA = atan2(a.y - centroid.y, a.x - centroid.x);
			float angleB = atan2(b.y - centroid.y, b.x - centroid.x);
			return angleA < angleB;
			};
		std::sort(vertices.begin(), vertices.end(), angleComparator);
		};

	sortVertices(leftVertices);
	sortVertices(rightVertices);

	bool isDelete = false;

	if (leftVertices.size() >= 3) {
		AddNewPolygon(leftVertices, originalColor, polygon, (rand() % 30 - 30) / 1000.0f);
		isDelete = true;
	}

	if (rightVertices.size() >= 3) {
		AddNewPolygon(rightVertices, originalColor, polygon, (rand() % 30) / 1000.0f);
		isDelete = true;
	}

	if (isDelete)
		DeletePolygonIndexList.push_back(polygonNum);
}

// 폴리곤들과 드래그 선분의 충돌 여부 확인 함수
bool IsLineIntersectingPolygons(std::vector<struct Polygon>& polygons, glm::vec3 lineStart, glm::vec3 lineEnd, std::vector<IntersectionInfo>& intersections) {
	glm::vec3 intersection;
	bool isIntersect = false;

	for (int i = 0; i < polygons.size(); ++i) {
		struct Polygon& polygon = polygons[i];
		int A[4] = { 0, 1, 3, 2 };
		int B[4] = { 1, 3, 2, 0 };
		int C[5] = { 0, 1, 3, 4, 2 };
		int D[5] = { 1, 3, 4, 2, 0 };

		switch (polygon.vertex_num) {
		case 3:
			for (int j = 0; j < polygon.vertex_num; ++j) {
				glm::vec3 v1 = vertexPosition[polygon.start_index + j];
				glm::vec3 v2 = vertexPosition[polygon.start_index + (j + 1) % polygon.vertex_num];

				if (LineSegmentsIntersect(lineStart, lineEnd, v1, v2, intersection)) {
					intersections.push_back({ i, j, intersection });
					isIntersect = true;
				}
			}
			break;
		case 4:
			for (int j = 0; j < 4; ++j) {
				glm::vec3 v1 = vertexPosition[polygon.start_index + A[j]];
				glm::vec3 v2 = vertexPosition[polygon.start_index + B[j]];

				if (LineSegmentsIntersect(lineStart, lineEnd, v1, v2, intersection)) {
					intersections.push_back({ i, A[i], intersection });
					isIntersect = true;
				}
			}
			break;
		case 5:
			for (int j = 0; j < 5; ++j) {
				glm::vec3 v1 = vertexPosition[polygon.start_index + C[j]];
				glm::vec3 v2 = vertexPosition[polygon.start_index + D[j]];

				if (LineSegmentsIntersect(lineStart, lineEnd, v1, v2, intersection)) {
					intersections.push_back({ i, C[i], intersection });
					isIntersect = true;
				}
			}
			break;
		}
	}
	return isIntersect;
}

// 폴리곤의 정점들 중 가장 y좌표가 낮은 정점 찾기 (바구니와 충돌 구현을 위해)
std::pair<glm::vec3, int> FindLowestVertexForPolygon(const struct::Polygon& polygon) {
	glm::vec3 lowestVertex = vertexPosition[polygon.start_index];
	int lowestIndex = polygon.start_index;

	for (int j = 1; j < polygon.vertex_num; ++j) {
		int index = polygon.start_index + j;
		if (vertexPosition[index].y < lowestVertex.y) {
			lowestVertex = vertexPosition[index];
			lowestIndex = index;
		}
	}

	return std::make_pair(lowestVertex, lowestIndex);
}

// 바구니와 충돌 여부 확인 함수
bool IsPointOnBucket(glm::vec3 A, glm::vec3 B, glm::vec3 P) {
	if (P.y <= A.y && P.x >= A.x && P.x <= B.x)
		return true;
	return false;
}

//폴리곤들과 충돌을 검사하고 이미 붙어있는 폴리곤들 바구니와 같이 이동시키기
void CheckLowestVertexForEachPolygon() {
	for (int i = 0; i < polygons.size(); ++i) {
		const struct::Polygon& polygon = polygons[i];
		auto result = FindLowestVertexForPolygon(polygon);
		glm::vec3 lowestVertex = result.first;
		int lowestPolygonIndex = result.second;

		if (IsPointOnBucket(vertexPosition[2], vertexPosition[3], lowestVertex)) {
			for (int j = 0; j < polygon.vertex_num; ++j) {
				int index = polygon.start_index + j;
				polygons[i].rotate = -1;
				vertexPosition[index].x += bucket_dx;
			}
		}
	}
}

GLvoid drawScene()
{
	glClearColor(1.0f, 0.8f, 0.8f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(shaderProgramID);
	glBindVertexArray(VAO);

	// 경로 그리기
	if(DrawPath){
		for (int i = 0; i < polygons.size(); ++i) {
			if (polygons[i].rotate == 1) {
				glColor3f(1.0f, 0.3f, 0.3f);
				glBegin(GL_LINE_STRIP);
				GLfloat tStep = 0.05f;
				for (GLfloat t = 0.0f; t <= 1.0f; t += tStep) {
					GLfloat x = (2 * t * t - 3 * t + 1) * polygons[i].p1.x + (-4 * t * t + 4 * t) * polygons[i].p2.x + (2 * t * t - t) * polygons[i].p3.x;
					GLfloat y = (2 * t * t - 3 * t + 1) * polygons[i].p1.y + (-4 * t * t + 4 * t) * polygons[i].p2.y + (2 * t * t - t) * polygons[i].p3.y;
					glVertex3f(x, y, 0.0f);
				}
				glEnd();
			}
		}
	}

	// 폴리곤과 바구니 그리기
	int currentIndex = 6;
	if (Line) {
		for (int i = 0; i < polygons.size(); ++i) {
			for (int j = 0; j < polygons[i].vertex_num - 2; ++j)
				glDrawArrays(GL_LINE_LOOP, currentIndex + j, 3);
			currentIndex += polygons[i].vertex_num;
		}
		glDrawArrays(GL_LINE_LOOP, 2, 3);
		glDrawArrays(GL_LINE_LOOP, 3, 3);
	}
	else {
		for (int i = 0; i < polygons.size(); ++i) {
			for (int j = 0; j < polygons[i].vertex_num - 2; ++j)
				glDrawArrays(GL_TRIANGLES, currentIndex + j, 3);
			currentIndex += polygons[i].vertex_num;
		}
		glDrawArrays(GL_TRIANGLES, 2, 3);
		glDrawArrays(GL_TRIANGLES, 3, 3);
	}

	// 마우스 드래그 선분 그리기
	if (isDrag) {
		glDrawArrays(GL_LINE_STRIP, 0, 2);
	}
	glutSwapBuffers();
}

GLvoid Keyboard(unsigned char key, int x, int y)
{
	switch (key) {
	case 'q':
		glutLeaveMainLoop();
		break;
	case 'Q':
		glutLeaveMainLoop();
		break;
	case 'l':
		Line = !Line;
		break;
	case 'L':
		Line = !Line;
		break;
	case'p':
		DrawPath = !DrawPath;
		break;
	case'P':
		DrawPath = !DrawPath;
		break;
	case 's':
		Speed += 5;
		break;
	case 'S':
		Speed -= 5;
		break;
	}
	InitBuffer();
	glutPostRedisplay();
}

void Mouse(int button, int state, int x, int y) {
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
		GLfloat X, Y;
		ScreenToOpenGL(x, y, X, Y); 
		isDrag = true;
		vertexPosition[0] = { X, Y, 0.0f };
		vertexPosition[1] = { X, Y, 0.0f };
	}
	else if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) {
		isDrag = false;
		std::vector<IntersectionInfo> intersections;
		if (IsLineIntersectingPolygons(polygons, vertexPosition[0], vertexPosition[1], intersections)) {
			// 폴리곤별로 교차 처리
			for (int p = 0; p < polygons.size(); ++p) {
				std::vector<IntersectionInfo> polygonIntersections;

				for (const auto& intersection : intersections) {
					if (&polygons[intersection.polygonIndex] == &polygons[p]) {
						polygonIntersections.push_back(intersection);
					}
				}

				// 교차하면 폴리곤 분할 함수 호출
				if (!polygonIntersections.empty()) {
					SplitPolygon(polygonIntersections, polygons[p], p);
				}
			}

			// 폴리곤 지우기
			for (int i = DeletePolygonIndexList.size() - 1; i >= 0; --i) {
				int startIdx = polygons[DeletePolygonIndexList[i]].start_index;
				int numVertices = polygons[DeletePolygonIndexList[i]].vertex_num;

				vertexPosition.erase(vertexPosition.begin() + startIdx, vertexPosition.begin() + startIdx + numVertices);
				vertexColor.erase(vertexColor.begin() + startIdx, vertexColor.begin() + startIdx + numVertices);

				polygons.erase(polygons.begin() + DeletePolygonIndexList[i]);
			}

			int currentIndex = 6;
			for (int i = 0; i < polygons.size(); ++i) {
				polygons[i].start_index = currentIndex;
				currentIndex += polygons[i].vertex_num;
			}

			DeletePolygonIndexList.clear();
		}
	}
	InitBuffer();
	glutPostRedisplay();
}

void Motion(int x, int y) {
	if (isDrag) {
		GLfloat X, Y;
		ScreenToOpenGL(x, y, X, Y);
		vertexPosition[1] = { X,Y,0.0f };
	}
	glutPostRedisplay();
}

void TimerFunction(int value)
{
	if (value == 1) { // 폴리곤 생성, 바구니 움직이기
		++TimeNum;
		if (TimeNum >= 30) {
			SetPolygon();
			TimeNum = 0;
		}

		for (int i = 0; i < 4; ++i) {
			vertexPosition[2 + i].x += bucket_dx;
		}
		if (vertexPosition[2].x <= -1.0f || vertexPosition[3].x >= 1.0f)
			bucket_dx *= -1;

		CheckLowestVertexForEachPolygon();

		InitBuffer();
		glutPostRedisplay();
		glutTimerFunc(60, TimerFunction, 1);
	}
	else if (value == 2) { // 회전 또는 떨어지는 폴리곤 애니메이션
		bool needResetIndex = false;

		for (int i = 0; i < polygons.size(); ++i) {
			if (polygons[i].rotate == 1) { 
				polygons[i].rotation_angle += 6.0f; 
				GLfloat t = polygons[i].move_num / 100.0f;
				polygons[i].point = {
					 (2 * t * t - 3 * t + 1) * polygons[i].p1.x + (-4 * t * t + 4 * t) * polygons[i].p2.x + (2 * t * t - t) * polygons[i].p3.x,
					  (2 * t * t - 3 * t + 1) * polygons[i].p1.y + (-4 * t * t + 4 * t) * polygons[i].p2.y + (2 * t * t - t) * polygons[i].p3.y
				};
				SetShape(polygons[i].vertex_num, polygons[i].start_index, polygons[i].point, polygons[i].rotation_angle);
				++polygons[i].move_num;
			}
			else if (polygons[i].rotate == 0) {
				FallShape(polygons[i].vertex_num, polygons[i].start_index, polygons[i].point, i);
				++polygons[i].move_num;
			}
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
			int currentIndex = 6;
			for (int i = 0; i < polygons.size(); ++i) {
				polygons[i].start_index = currentIndex;
				currentIndex += polygons[i].vertex_num;
			}
		}

		InitBuffer();
		glutPostRedisplay();
		glutTimerFunc(Speed, TimerFunction, 2);
	}
}

void main(int argc, char** argv) 
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
	glutTimerFunc(60, TimerFunction, 2);
	glutMouseFunc(Mouse);
	glutMotionFunc(Motion);
	glutMainLoop();
}