//********************************************************************************************
//MAPSPAWN.nut is called on newgame or transitions
//********************************************************************************************
printl("==== calling mapspawn.nut");
//DoIncludeScript("mycoopmapspawn",Mycoopmapspwawn());
function Wustmapspwawn()
{
	if(GetMapName() == "wust1")
	{
		printl("==== calling Wustmapspwawn.nut");
		//local mapcommand = Entities.CreateByClassname("point_clientcommand");
		EntFire("@relay_pti_level_end", "AddOutput", "Ontrigger mapcommand:command:changelevel wust2");//没有@command 和 @changelevel
	}
	if(GetMapName() == "wust2")
	{
		EntFire("@relay_pti_level_end", "AddOutput", "Ontrigger @command:command:changelevel wust3");
	}
	if(GetMapName() == "wust3")
	{
		EntFire("@relay_pti_level_end", "AddOutput", "Ontrigger mapcommand:command:changelevel wust4");
		//EntFire( "@changelevel", "Changelevel", next_map, 0.0 )
	}
	if(GetMapName() == "wust4")
	{
		//local mapcommand = Entities.CreateByClassname("point_clientcommand");
		EntFire("@relay_pti_level_end", "AddOutput", "Ontrigger mapcommand:command:changelevel wust5");
	}
	if(GetMapName() == "wust5")
	{
		//local mapcommand = Entities.CreateByClassname("point_clientcommand");
		EntFire("@relay_pti_level_end", "AddOutput", "Ontrigger mapcommand:command:changelevel wust6");
	}
}

Wustmapspwawn();