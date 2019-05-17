#include <SFML\Graphics.hpp>
#include <iostream>
#include <cstdlib>
#include <ctime>

#define NORM_GRAY 0

#define PI 3.14159265359
#define DEG_TO_RAD 0.01745329252

using namespace std;
using namespace sf;

class Field {
public:

	uint16_t width, height;
	float noise;

	Field() {
		width = 0;
		height = 0;
	}

	Field(Image img, uint8_t type = NORM_GRAY) {
		Vector2u sz = img.getSize();
		width = sz.x;
		height = sz.y;
		ptr = new float[width * height];
		const Uint8 *p = img.getPixelsPtr();
		if (type == NORM_GRAY) {
			for (uint32_t i = 0; i < width * height; i++) {
				ptr[i] = float(p[i * 4] + p[i * 4 + 1] + p[i * 4 + 2]) / 765.0f;
			}
		}
		noise = 0.1;
	}

	float getPoint(uint16_t x, uint16_t y) {
		uint16_t rnd = rand();
		int16_t r1 = rnd % 256, r2 = (rnd >> 8) % 128;
		return ptr[y * width + x] + (0.5 - float(rand()) / RAND_MAX) * noise;
	}

private:

	float* ptr;
};

class Robot {
public:

	Field field;
	uint8_t sensors, outputs;
	float angle, speed, anglePlus, expAngleK, angleOld, rate, width, height;
	Vector2f pos;
	Vector2f sensorPos;
	Vector2f sPos;
	uint16_t tt;
	uint32_t contT;
	
	Robot(Field& field, Image img, uint8_t sensors = 1, uint8_t outputs = 1) {
		this->field = field;
		this->sensors = sensors;
		this->outputs = outputs;
		pos = Vector2f(61, 220);
		angle = 90;
		anglePlus = 0;
		expAngleK = 0.3;
		speed = -2;
		sensorPos = Vector2f(19, 19);
		rate = 0;
		contT = 0;
		tt = 0;

		Vector2u sz = img.getSize();
		width = sz.x;
		height = sz.y;
		ptr = new uint8_t[width * height];
		const Uint8 *p = img.getPixelsPtr();
		for (uint32_t i = 0; i < width * height; i++) {
			ptr[i] = ((p[i * 4] + p[i * 4 + 1] + p[i * 4 + 2]) == 765) ? 1 : 0;
		}
	}

	Clock clock;

	void render() {
		clock.restart();
		float ks = sin(angle * DEG_TO_RAD), kc = cos(angle * DEG_TO_RAD);
		pos.x += speed * kc;
		pos.y += speed * ks;
		Vector2f tsp = sensorPos;
		tsp.x = -tsp.x * kc;
		tsp.y = -tsp.y * ks;
		sPos = pos + tsp;
		angleOld = angle;
		float val = field.getPoint(uint16_t(sPos.x), uint16_t(sPos.y));

		float len = 15;
		float ksR = sin((angle - 20) * DEG_TO_RAD), kcR = cos((angle - 20) * DEG_TO_RAD);
		float ksL = sin((angle + 45) * DEG_TO_RAD), kcL = cos((angle + 45) * DEG_TO_RAD);
		Vector2f posSL, posSR;
		posSL.x = -len * kcL;
		posSL.y = -len * ksL;
		posSR.x = -len * kcR;
		posSR.y = -len * ksR;
		int controller = abs(speed) + 0.5;
		float stepSize = max(abs(posSR.y - posSL.y), abs(posSR.x - posSL.x));
		float k1 = (posSR.x - posSL.x) / stepSize, k2 = (posSR.y - posSL.y) / stepSize;
		for (uint16_t index = 0; index < stepSize; posSL.x += k1, posSL.y += k2, index++) {
			uint32_t addr = uint32_t(pos.x + posSL.x) + uint32_t(pos.y + posSL.y) * width;
			uint8_t pix = ptr[addr];
			if (!pix) {
				rate += 1;
				ptr[addr] = 2;
				uint32_t px = pos.x + posSL.x, py = pos.y + posSL.y;
				for (int x = -controller; x <= controller; x++) {
					for (int y = -controller; y <= controller; y++) {
						addr = (px + x) + (py + y) * width;
						pix = ptr[addr];
						if (!pix) rate += 1;
						ptr[addr] = 2;
					}
				}
				break;
			}
		}
		/*
		for (int x = -5; x <= 5; x++) {
			for (int y = -5; y <= 5; y++) {
				uint32_t addr = (uint32_t(round(pos.x)) + x) + uint32_t(round(pos.y) + y) * width;
				uint8_t pix = ptr[addr];
				if (!pix) rate += 1;
				ptr[addr] = 2;
			}
		}
		*/
		//cout << "rate:" << rate << endl;

		anglePlus = run(val) * expAngleK + anglePlus * (1 - expAngleK);
		angle += anglePlus;
		if (angle > 360) angle -= 360;
		if (angle < 0) angle += 360;
		contT += clock.getElapsedTime().asMicroseconds();
		cout << "time:" << (contT / 1000) << endl;
	}

	float run(float inputs) {
		return (inputs - 0.5) * 20;
	}
private:
	uint8_t *ptr;
};

int main() {
	srand(time(NULL) & 14);

	Image img;
	img.loadFromFile("../../pol.png");
	
	Field field(img);
	cout << "------------------" << endl;

	Image imgLine;
	imgLine.loadFromFile("../../pol2.png");

	Robot robot(field, imgLine);

	RectangleShape zone;
	zone.setSize(Vector2f(10, 10));
	zone.setOrigin(Vector2f(5, 5));
	zone.setFillColor(Color(0, 0, 255, 100));
	RectangleShape roboShape;
	roboShape.setSize(Vector2f(20, 16));
	roboShape.setOrigin(Vector2f(10, 8));
	roboShape.setFillColor(Color(255, 0, 0));	
	CircleShape sen;
	sen.setRadius(3);
	sen.setOrigin(Vector2f(sen.getRadius(), sen.getRadius()));
	sen.setFillColor(Color(0, 255, 0));
	Texture texture;
	texture.loadFromImage(imgLine);

	Sprite sprite;
	sprite.setTexture(texture);

	Vector2u sz = img.getSize();
	RenderWindow window(VideoMode(sz.x,sz.y), "Win");
	window.setVerticalSyncEnabled(true);

	Event event;
	while (window.isOpen()) {
		while (window.pollEvent(event)) {
			if (event.type == Event::Closed) window.close();
		}

		window.clear();

		window.draw(sprite);

		robot.render();
		roboShape.setPosition(robot.pos); 
		zone.setPosition(robot.pos);
		sen.setPosition(robot.sPos);
		roboShape.setRotation(robot.angleOld);
		window.draw(roboShape);
		window.draw(zone);
		window.draw(sen);
		window.display();
	}
	return 0;
}