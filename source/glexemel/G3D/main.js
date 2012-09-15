function init() {
	/*
	struct FileHeader{
		uint8 id[3];
		uint8 version;
	};
	*/
	var fileHeader = struct({
		id      : array(uint8(), 3),
		version : uint8()
	})

	/*
	struct ModelHeader{
		uint16 meshCount;
		uint8 type;
	};
	*/
	var modelHeader = struct({
		meshCount : uint16(),
		type      : uint8()
	})

	/*
	enum MeshPropertyFlag{
		mpfCustomColor= 1,
		mpfTwoSided= 2,
		mpfNoSelect= 4
	};
	*/
	var meshPropertyFlags = struct({
		customColor : bitfield("bool", 1),
		twoSided    : bitfield("bool", 1),
		noSelect    : bitfield("bool", 1),
		padding     : bitfield("unsigned", 29)
	})
	
	//only 0x1 is currently used, indicating that there is a diffuse texture in this mesh.
	var textureFlags = struct({
		diffuseTexture : bitfield("bool", 1),
		padding        : bitfield("unsigned", 31)
	})
	
	/*
	struct MeshHeader{
		uint8 name[64];
		uint32 frameCount;
		uint32 vertexCount;
		uint32 indexCount;
		float32 diffuseColor[3];
		float32 specularColor[3];
		float32 specularPower;
		float32 opacity;
		uint32 properties;
		uint32 textures;
	};
	*/
	var meshHeader = struct({
		name          : array(uint8(), 64),
		frameCount    : uint32(),
		vertexCount   : uint32(),
		indexCount    : uint32(),
		diffuseColor  : array(float(), 3),
		specularColor : array(float(), 3),
		specularPower : float(),
		opacity       : float(),
		properties    : meshPropertyFlags,
		textures      : textureFlags
	})

	/*
	A list of uint8[64] texture name values. One for each texture in the mesh. If there are no textures in the mesh no texture names are present. In practice since Glest only uses 1 texture for each mesh the number of texture names should be 0 or 1.
	*/
	var textureName = struct({
		TextureName : array(uint8(), 64)
	})

	// some helper
	var coords = struct({
		x : float(),
		y : float(),
		z : float()
	})
	var tcoords = struct({
		s : float(),
		t : float()
	})
	/*
	After each mesh header and texture names the mesh data is placed.

	vertices: frameCount * vertexCount * 3, float32 values representing the x, y, z vertex coords for all frames
	normals: frameCount * vertexCount * 3, float32 values representing the x, y, z normal coords for all frames
	texture coords: vertexCount * 2, float32 values representing the s, t tex coords for all frames (only present if the mesh has 1 texture at least)
	indices: indexCount, uint32 values representing the indices
	*/

	var meshData = struct({
		TextureName : textureName,
		vertices    : array(coords, 1),  //length = frameCount*vertexCount
		normals     : array(coords, 1),  // -''-
		texcoords   : array(tcoords, 1), //length = vertexCount
		indices     : array(uint32(), 1) //length = indexCount
	})
	//FIXME: changing type doesn't work -> useless
// 	var meshData_notex = struct({
// 		vertices  : array(coords, 1),  //length = frameCount*vertexCount
// 		normals   : array(coords, 1),  // -''-
// 		indices   : array(uint32(), 1) //length = indexCount
// 	})

	// general structures
	var mesh = struct({
		MeshHeader  : meshHeader,
		MeshData    : meshData
	})
	var g3d = struct({
		FileHeader  : fileHeader,
		ModelHeader : modelHeader,
		Meshes      : array(mesh, 1) //length = meshCount
	})

	//texture stuff is conditional
	// array length = 0 doesn't work, and i don't know any other conditional way
	// so we have copy&paste-fun
	
	// array length depends on meshHeader -> update dynamically
	//set an update function (which gets called everytime the structure changes, i.e. cursor moved)
	g3d.child("Meshes").updateFunc = function(mainStruct) {
		this.length = this.parent.ModelHeader.meshCount.value;
	}
	//FIXME: changing type doesn't work -> always with texture
// 	mesh.child("MeshData").updateFunc = function(mainStruct) {
// 		var t = this.parent.MeshHeader.textures.diffuseTexture.value;
// 		if(t==true){
// 			this.type = meshData;
// 		}else{
// 			this.type = meshData_notex;
// 		}
// 		this.parent.updateFunc(mainStruct);
// 	}
	meshData.updateFunc = function(mainStruct) {
		var mh = this.parent.MeshHeader;
		var len = mh.frameCount.value * mh.vertexCount.value;
		this.vertices.length = len;
		this.normals.length = len;
		this.texcoords.length = mh.vertexCount.value;
		this.indices.length = mh.indexCount.value;
	}
	//FIXME: changing type doesn't work -> useless
// 	meshData_notex.updateFunc = function(mainStruct) {
// 		var mh = this.parent.MeshHeader;
// 		var len = mh.frameCount.value * mh.vertexCount.value;
// 		this.vertices.length = len;
// 		this.normals.length = len;
// 		this.indices.length = mh.indexCount.value;
// 	}

	return g3d;
}
