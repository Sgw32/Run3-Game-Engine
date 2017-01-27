
#define ADD_AENT(x,y) pElement = XMLNode->FirstChildElement(x); while(pElement) {y(pElement);pElement = pElement->NextSiblingElement(x);}