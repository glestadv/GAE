#include "../shared_lib/include/xml/xml_parser.h"
#include <string>

using namespace std;
using namespace Shared::Xml;

int main()
{
	try 
	{
		XmlTree	xmlTree;
		const string path = "GUILayout.xml";

		xmlTree.load(path);
		const XmlNode *treeNode = xmlTree.getRootNode();

		treeNode->getChild("Texture");

		XmlIo io = XmlIo::getInstance();
/*const char *test = "<?xml version=\"1.0\" standalone=\"no\"?> \
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
  </Texture>\0";*/
const char *test = " \0";
		const XmlNode *parsedNode = io.parseString(test);

		parsedNode->getChild("Texture");

	} catch (exception &e) {
		cout << e.what() << endl;
	}

	system("Pause");

	return 0;
}