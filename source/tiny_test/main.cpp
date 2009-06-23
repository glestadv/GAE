#include "../shared_lib/include/xml/xml_parser.h"
#include <string>
#include <iostream>

using namespace std;
using namespace Shared::Xml;

int main()
{
	try
	{
		XmlTree	xmlTree;
		/*const string path = "GUILayout.xml";

		xmlTree.load(path);
		const XmlNode *treeNode = xmlTree.getRootNode();

		treeNode->getChild("Texture");
*/
		XmlIo io = XmlIo::getInstance();
const char *test = "<?xml version=\"1.0\" standalone=\"no\"?> \
\
<Panel visible     = \"true\"\
       name        = \"Main Frame\">\
\
  <Texture  type        = \"TEXTURE_2D\"\
            path        = \"GUIElements.PNG\" \
            mode        = \"MODULATE\"\
            mipmap      = \"true\">\
\
    <Wrap   s           = \"REPEAT\"\
            t           = \"REPEAT\" />\
\
    <Filter mag         = \"LINEAR\" \
            min         = \"LINEAR_MIPMAP_LINEAR\" />\
  </Texture>\n</Panel>\0";
//const char *test = "\0"; //empty test
		const XmlNode *parsedNode = io.parseString(test);

		io.save("test.xml", parsedNode);

		//const string path = "air_ballista.xml"; // xml comment test
		const string path = "test.xml"; // test the saved xml text
		
		xmlTree.load(path);
		
		const XmlNode *treeNode = xmlTree.getRootNode();
		treeNode->getChild("Texture");

		cout << xmlTree.toString() << endl;

		xmlTree.parse(xmlTree.toString());

		treeNode->getChild("Texture3");

	} catch (exception &e) {
		cout << e.what() << endl;
	}

	system("Pause");

	return 0;
}