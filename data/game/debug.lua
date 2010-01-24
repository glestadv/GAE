function debugPathfinder()
	debugSet("ResourceMap=off");
	debugSet("RegionColouring=off");
	
	debugSet("DebugField=land");
	
	debugSet("AStarTextures=on");
	debugSet("ClusterOverlay=on");
end

function debugOff()
	debugSet("ResourceMap=off");
	debugSet("RegionColouring=off");
	debugSet("AStarTextures=off");
	debugSet("ClusterOverlay=off");
	debugSet("TransitionEdges=clear");
end

function showResourceMap(res)
	debugSet("ResourceMap="..res);
	debugSet("RegionColouring=off");
	debugSet("ClusterOverlay=off");
	debugSet("TransitionEdges=clear");
end

function clearEdges()
	debugSet("TransitionEdges=clear");
end

function showEdges(x, y)
	debugSet("TransitionEdges="..x..","..y..":".."all");
end

function showSouthEdges(x, y) 
	debugSet("TransitionEdges="..x..","..y..":".."south");
end

function showEastEdges(x, y) 
	debugSet("TransitionEdges="..x..","..y..":".."east");
end

function showNorthEdges(x, y) 
	debugSet("TransitionEdges="..x..","..y..":".."north");
end

function showWestEdges(x, y) 
	debugSet("TransitionEdges="..x..","..y..":".."west");
end
