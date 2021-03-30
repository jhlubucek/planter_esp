#include <Arduino.h>

class AnalogSensor
{
private:
    int pin;
    int min;
    int max;
    double lastValue;
    int controlPin;
    double mesureValue();
    bool invert;
public:
    AnalogSensor(int pin, int min, int max, int controlPin = 0);
    double getValue();
    double getLastValue();
    void setInvert(bool invert);
};


AnalogSensor::AnalogSensor(int pin, int min, int max, int controlPin){
    this->pin = pin;
    this->min = min;
    this->max = max;
    this->controlPin = controlPin;
    this->invert = false;
}


/**
 *  vrací hodnotu jako procento v rozpětí min-max
 */
double AnalogSensor::getValue(){
    int value = mesureValue();
    value = (value > max) ? max : value;
    value = (value < min) ? min : value;
    lastValue = (value - min)/(double)(max - min)*100;

    if (this->invert)
    {
        lastValue = 1 - lastValue;
    }
    
    return lastValue;
};

double AnalogSensor::mesureValue(){
    double value = 0;
    if (controlPin)
    {
        digitalWrite(SENSOR_SWITCH_PIN, HIGH);
        for (int i = 0; i < 10; i++){
            delay(50);
            value += analogRead(pin);
        }
        digitalWrite(SENSOR_SWITCH_PIN, LOW);
    }else{
        for (int i = 0; i < 10; i++){
            delay(50);
            value += analogRead(pin);
        }
    }
    

    return value/10;
}

double AnalogSensor::getLastValue(){
    return lastValue;
}

void AnalogSensor::setInvert(bool invert){
    this->invert = invert;
}