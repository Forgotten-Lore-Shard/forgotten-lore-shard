include "include/say";
include ":attributes:attributes";
include ":itemUtils:itemdesc";
include ":gumps:yesno";
include ":traps:traps";
include ":charactercreation:habilidades";


function examinarAlvo(who)
	SendSysMessageEx(who, "Selecione um alvo.", SSM_REQUEST);
	var targ := Target(who);
	if( !targ )
		SendSysMessageEx(who, "Cancelado.", SSM_FAIL);
		return 0;
	elseif ( !targ.isA(POLCLASS_MOBILE) && !targ.isA(POLCLASS_CONTAINER) && !targ.isA(POLCLASS_DOOR) )
		SendSysMessageEx(who, "Alvo invalido.", SSM_FAIL);
		return 0;
	endif

	var armadilhaRevelada := 0;
	var acharamArmadilha := array;
	var found := 0;
	if ( GetObjProperty(targ, "disfarce") )
		if ((valorInicial(who, AWARENESS) >= 40) && (SkillCheck(who, AWARENESS, 70) > 0))
			found:=1;
			var char := GetObjProperty(targ, "chardata");
			var name := char.FirstName;
			var last_name := char.LastName;
			if (last_name)
				name := name+" "+last_name;
			endif
			SendSysMessageEx(who, "Voce descobriu disfarce de "+name+" em "+targ.name+".", SSM_INFO);
			SendSysMessageEx(who, "ATENCAO: Se voce nao conhece esse personagem, ignore o fato de saber o nome dele.", SSM_INFO);
		endif
		return 0;
	elseif ( GetObjProperty(targ, "TrapList") )
		if(!ReserveItem(targ)) //Apenas um personagem examina o bau por vez.
	  		SendSysMessage(who, "Alguem ja esta examinando "+targ.name+".");
	  		return 0;
	  	else
	  		PrintTextAbove(who, "*Examinando "+targ.name+"*");
	  		sleep(1);
		endif
		if(GetObjProperty(targ, "trapSpoted"))
			armadilhaRevelada := GetObjProperty(targ, "trapSpoted"); //CPROP que diz quem revelou a trap pra todos
		endif
		if(GetObjProperty(targ, "acharamArmadilha"))
			acharamArmadilha := GetObjProperty(targ, "acharamArmadilha"); //Lista de pessoas que revelaram a armadilha só para si
		endif
		if ((!armadilhaRevelada) && (who.serial in acharamArmadilha)) //Quando você achou ela mas não revelou pra todos
			SendSysMessageEx(who, "Armadilha ja foi encontrada e esta obvia para voce.", SSM_INFO);
			if (YesNo(who, "Deseja revelar a armadilha para todos?", "Sim.", "Nao, deixa so pra mim."))
				PrintText(targ, "*armadilha revelada para todo por "+who.name+"*");
				TrapFound(targ, who);
			endif
			return 1;
		elseif(armadilhaRevelada)
			SendSysMessageEx(who, "Armadilha ja foi encontrada e esta obvia para todos.", SSM_INFO);
			return 1;
		else
			var diff;
			var lvl := Cint(GetObjProperty(targ, "level"));
			case (lvl)
				1: diff := 15;
				2: diff := 40;
				3: diff := 60;
				4: diff := 86;
				5: diff := 90;
				6: diff := 110;
				default: diff := 40;
			endcase
			if ( SkillCheck(who, AWARENESS, diff) > 0 )
				acharamArmadilha.Append(who.serial);
				SetObjProperty(targ, "acharamArmadilha", acharamArmadilha);
				SendSysMessage(who,"Armadilha revelada em "+targ.name+".");
				return 1;
			else
				SendSysMessageEx(who, "Voce nao encontrou nada de diferente.", SSM_INFO);
				return 0;
			endif
		endif
	else
		SendSysMessageEx(who, "Voce nao encontrou nada de diferente.", SSM_INFO);
		return 0;
	endif
	return 0;
endfunction

function CalculateDHDiff(who, mobile)

	var mobile_skill := CInt(AP_GetSkill(mobile, SNEAK));

	//sendsysmessage(mobile, "dificuldade de  "  + who.name + "te achar " + mobile_skill);
	if (GetObjProperty(mobile, "#overridehiding") != error)
		mobile_skill := cint(GetObjProperty(mobile, "#overridehiding"));
	endif
	var dist := Distance(who, mobile);
	var difficulty;

	if (TemHabilidade(mobile, "Comunhao com as Sombras"))
		difficulty := CInt(mobile_skill);
	else
		if (dist >=8)
			difficulty := CInt(mobile_skill);
		elseif (dist >= 7)
			difficulty := CInt(mobile_skill*0.95);
		elseif (dist >= 6)
			difficulty := CInt(mobile_skill*0.9);
		elseif ( dist >= 5 )
			difficulty := CInt(mobile_skill*0.85);
		elseif ( dist >= 4 )
			difficulty := CInt(mobile_skill * 0.8);
		elseif ( dist >= 3 )
			difficulty := CInt(mobile_skill * 0.7);
		elseif ( dist >= 2 )
			difficulty := CInt(mobile_skill * 0.6);
		elseif ( dist >= 1 )
			difficulty := CInt(mobile_skill * 0.5);
		else
			difficulty := CInt(mobile_skill * 0.25);
		endif
	endif

	return difficulty;

endfunction

function examinarArea(who)
	var range := CInt( AP_GetSkill(who, AWARENESS) / 10 ) + 1;

	var vantagem := (GetObjProperty(who, "chardata")).vantagemracial;
	if ( vantagem == "Sentidos Aguçados" )
		range := range + 5;
	endif

/*	var raca := GetObjProperty(who, "raca");
	if ( raca == "elfo" )
		range := range + 10;
	elseif ( raca == "meio-elfo" || raca == "drow" )
		range := range + 5;
	endif
*/

	var diff;
	var lvl;
	var found := 0;
	foreach mobile in ListMobilesNearLocationEX(who.x, who.y, who.z, range, LISTEX_FLAG_HIDDEN)
		if ( mobile == who ); // Do Nothing
		elseif (!mobile.npctemplate && CInt(GetObjProperty(who, "IgnoreHidden"))); // Do Nothing
        elseif (who.party && (who.party.leader.name == mobile.party.leader.name)); //se esta na mesma party nao checa
		elseif ( CheckLineOfSight(who, mobile) && abs(mobile.z - who.z ) < 19 )
			var difficulty := CalculateDHDiff(who, mobile);
			if ( SkillCheck(who, AWARENESS, difficulty) > 0 )
				found := found+1;
				mobile.hidden := 0;
				PrintText(mobile, "*aparece*");
				SendSysMessageEx(who, "Voce encontrou "+mobile.name+".", SSM_INFO);
				SendSysMessageEx(mobile, who.name+" te encontrou!", SSM_FAIL);
			endif
		endif
		sleepms(2);
	endforeach

	foreach item in ListItemsNearLocation(who.x, who.y, who.z, range)
		if ( item.invisible )
			if ( item.isTrap())
				lvl := Cint(GetObjProperty(item, "level"));
				case (lvl)
					1: diff := 15;
					2: diff := 40;
					3: diff := 60;
					4: diff := 86;
					5: diff := 90;
					6: diff := 110;
					default: diff := -1; //testa contra a propria skill
				endcase
				if ( SkillCheck(who, AWARENESS, diff) > 0 )
					found := found+1;
					item.invisible := 0;
					SendSysMessageEx(who, "Voce encontrou "+item.desc+".", SSM_INFO);
					PrintTextAbovePrivate(item, "*!*", who);
				endif
			else
				var diff := CInt(GetObjProperty(item, "FindDiff"));
				if ( diff )
					if ( SkillCheck(who, AWARENESS, diff) > 0 )
						found := found+1;
						item.invisible := 0;
						SendSysMessageEx(who, "Voce encontrou "+item.desc+".", SSM_INFO);
						PrintTextAbovePrivate(item, "*!*", who);
					endif
				endif
			endif
		endif
		sleepms(2);
	endforeach
	return found;
endfunction
