/*#########################################################################
#                                                                         #
#   Simple script for testing the scriptable service browser              #
#   by creating a simple static browser with some cool radio              #
#   streams. URLs shamelessly stolen from Cool-Streams.xml.               #
#                                                                         #
#   Copyright                                                             #
#   (C) 2007, 2008 Nikolaj Hald Nielsen  <nhnFreespirit@gmail.com>        #
#   (C)       2008 Peter ZHOU <peterzhoulei@gmail.com>                    #
#   (C)       2008 Mark Kretschmann <kretschmann@kde.org>                 #
#                                                                         #
#   This program is free software; you can redistribute it and/or modify  #
#   it under the terms of the GNU General Public License as published by  #
#   the Free Software Foundation; either version 2 of the License, or     #
#   (at your option) any later version.                                   #
#                                                                         #
#   This program is distributed in the hope that it will be useful,       #
#   but WITHOUT ANY WARRANTY; without even the implied warranty of        #
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
#   GNU General Public License for more details.                          #
#                                                                         #
#   You should have received a copy of the GNU General Public License     #
#   along with this program; if not, write to the                         #
#   Free Software Foundation, Inc.,                                       #
#   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         #
##########################################################################*/

function Station( name, url, description )
{
    this.name = name;
    this.url = url;
    this.description = description;
}


categories = new Object;


var p4Desc = "P4 er den største og mest aflyttede danske radiokanal med et mix af landsdækkende og regionale programmer. Fra DRs 11 regioner følger P4 lytterne døgnet rundt med et alsidigt udbud af moderne public service radio.<p>Fra DRs 11 regioner hjælper vi dig igennem din dag med god musik; vi giver dig både de brede og de hurtige nyheder, og så kan vi hjælpe dig på vej i trafikken.<p>P4 - nærmest din egen radio.";


categories["Danmarks Radio"]= new Array (
    new Station( "P1", "http://wmscr1.dr.dk/e02ch01m?wmcontentbitrate=300000", "P1 er den moderne oplysningskanal med nyheder og samfundsstof, der skaber indsigt. Her sættes politik og hverdag til debat i interviews, analyser og reportager. P1 søger årsager og tendenser bag de aktuelle nyheder og sætter begivenhederne i perspektiv.<p>Man behøver ikke sidde stille og lytte i timevis for at få noget ud af P1. Vi går grundigt til værks og kommer godt i dybden, men der er mulighed for at stå af og på undervejs. Rigtig mange af P1s programmer kan downloades og høres som podcasting - hvor du vil og når du vil." ),
    new Station( "P2", "http://wmscr1.dr.dk/e02ch02m?wmcontentbitrate=300000", "P2 er radiokanalen for dig som har musik og kultur som en vigtig del af dit liv, og som gerne vil opleve og følge med i alt det der sker.<p>P2 er for dig som elsker god musik, hvad enten den er klassisk, jazz, verdensmusik eller alt det spændende ind imellem.<p>På P2 bliver du hver eftermiddag i P2 Plus opdateret på alt det der sker i kulturen lige nu: film, litteratur, musik, teater, kunst og det du ikke anede eksisterede. I P2s magasinprogrammer i weekenden kan du gå mere i dybden med de dele af kulturen, der interesserer dig mest.<p>P2 kan du bruge både til fordybelse og som gå-til-og-fra-radio på din vej gennem dagen derhjemme eller på arbejdet, på pc'en, i bilen, i toget.." ),
    new Station( "P3", "http://wmscr1.dr.dk/e02ch03m?wmcontentbitrate=300000", "P3 er radiokanalen for moderne unge og voksne, der kan lide at blive udfordret - både musikalsk og indholdsmæssigt.<p>Musikken på P3 er et alsidigt og varierende mix af ny spændende musik, morgendagens stjerner, nutidens hits og banebrydende klassikere.<p>Journalistisk sender P3 både P3 Nyheder klokken hel og uddybende interviews morgen og eftermiddag, ofte med usædvanlige vinkler på nutidige og aktuelle emner.<p>Satiren på P3 er ikke kun sjov og fis. P3s satire har vid og bid, og kradser i både aktuelle og klassiske emner som optager danskerne.<p>Sporten byder på intens og direkte dækning af de store begivenheder i både ind- og udland, samtidig med at der informeres bredt om mange idrætsgrene. " ),
    new Station( "P4 Bornholm", "http://wmscr1.dr.dk/e04ch08m?wmcontentbitrate=300000", p4Desc ),
    new Station( "P4 Fyn", "http://wmscr1.dr.dk/e04ch06m?wmcontentbitrate=300000", p4Desc ),
    new Station( "P4 København", "http://wmscr1.dr.dk/e04ch09m?wmcontentbitrate=300000", p4Desc ),
    new Station( "P4 Midt & Vest", "http://wmscr1.dr.dk/e04ch02m?wmcontentbitrate=300000", p4Desc ),
    new Station( "P4 Nordjylland", "http://wmscr1.dr.dk/e04ch01m?wmcontentbitrate=300000", p4Desc ),
    new Station( "P4 Sjælland", "http://wmscr1.dr.dk/e04ch07m?wmcontentbitrate=300000", p4Desc ),
    new Station( "P4 Syd", "http://wmscr1.dr.dk/e04ch05m?wmcontentbitrate=300000", p4Desc ),
    new Station( "P4 Trekanten", "http://wmscr1.dr.dk/e04ch04m?wmcontentbitrate=300000", p4Desc ),
    new Station( "P4 Østjyllands Radio", "http://wmscr1.dr.dk/e04ch03m?wmcontentbitrate=300000", p4Desc ),
    new Station( "P5", "http://wmscr1.dr.dk/e06ch01m?wmcontentbitrate=300000", "P5 byder på folkekære værter som Nis Boesdal, Jørgen de Mylius, Hans Otto Bisgaard, Søren Dahl, Karlo Staunskjær og Margaret Lindhardt.<p>Der er plads til både viser, dansktop, populærmusik og mulighed for at genhøre det bedste fra DR’s arkiver. Det sker i programmer som Dansktoppen, Gyldne Genhør, Giro 413, Eldorado, Nis på DAB og Cafe Hack." ),
    new Station( "DR Allegro", "http://wmscr1.dr.dk/e06ch03m?wmcontentbitrate=300000", "På DR Allegro finder du velkendt og populær klassisk musik. Foruden orkestermusik, klavermusik og kendte operaarier finder du filmmusik, musicalmelodier og danske sange.<p>Danske komponister og danske musikere er også rigt repræsenteret på kanalen, som kan høres på enhver pc verden over." ),
    new Station( "DR Barometer", "http://wmscr1.dr.dk/e02ch09m?wmcontentbitrate=300000", "DR Barometer - hangout for musikalske soulmates og postcentral for folk med noget på hjerte.<p>Med fokus på indierock, indietronica, alternativ country og elektronica præsenterer kanalen en perlerække af danske og internationale bands hentet fra universet omkring det legendariske radioprogram Det Elektriske Barometer på P3.<p>Kanalen spiller aktuelle Barometerhits blandet op med klassikere fra programmets 20 årige historie - alt sammen krydret med lytternes stemmer, ønsker og tanker fra tankevæggen, samt genudsendelser af nye og gamle Barometerudsendelser." ),
    new Station( "DR MGP", "http://wmscr1.dr.dk/e06ch09m?wmcontentbitrate=300000", "DR MGP er kanalen hvor du kan høre alle sangene fra MGP - krydret med de andre sange som MGP stjernerne også har lavet. Mathias, Amalie, SEB, Nicolai og alle de andre på din egen MGP kanal." ),
    new Station( "DR Boogieradio", "http://wmscr1.dr.dk/e02ch07m?wmcontentbitrate=300000", "DR Boogieradio spiller alle de hits du kender fra Boogie på tv og net. Søde Sys Bjerre og Jeppe \"fingerslap\" Voldum er dine værter, og ligesom musikken kan du altid høre dem lige her, 24 timer i døgnet! Du kan også genhøre ugens aktuelle Boogieliste og de bedste interviews fra tv." ),
    new Station( "DR Country", "http://wmscr1.dr.dk/e06ch06m?wmcontentbitrate=300000", "DR Country er hjemsted for countrygenren i den klassiske forstand.<p>Genren fortolkes bredt på kanalen, så du finder eksempelvis også de relaterede genrer bluegrass, countryrock og singer/songwriters. DR Country indeholder en blanding af god ny country og genrens ældre klassikere. " ),
    new Station( "DR Dansktop", "http://wmscr1.dr.dk/e02ch11m?wmcontentbitrate=300000", "DR Dansktop er kanalen for alle dem, der holder af dansktopmusik.<p>Hele døgnet sender DR Dansktop alle de bedste sange indenfor genren. Du kan høre dansktopmusikken, som den lyder i dag, men også høre ældre hits.<p>Selvom fire ud af fem sange på DR Dansktop er danske, kan du også høre svensk og tysk musik. Både nyere musik fra de to lande samt ældre numre, der har inspireret dansktoppens danske hits. " ),
    new Station( "DR Electronica", "http://wmscr1.dr.dk/e06ch10m?wmcontentbitrate=300000", "DR Electronica er for lyttere med åbne ører, hang til elektroniske klange og rytmiske finurligheder. Både feinschmeckere, nybegyndere og nysgerrige kan være med i dette mindre kluborienterede univers.<p>DR Electronica er håndplukkede danske og internationale produktioner, der gør en forskel i dag og i fremtiden." ),
    new Station( "DR Evergreen", "http://wmscr1.dr.dk/e06ch07m?wmcontentbitrate=300000", "DR Evergreen er for alle - unge som ældre - med en forkærlighed for den klassiske fortolkning af kendte populærmelodier - evergreens.<p>På kanalen finder du danske og udenlandske evergreens, primært inden for easy listening-genren , ofte fremført med sangsolist akkompagneret af orkester, strygere og kor. Nyere indspilninger med yngre kunstnere, som stilmæssigt passer ind i kanalen, er også repræsenteret på DR Evergreen." ),
    new Station( "DR Folk", "http://wmscr1.dr.dk/e06ch11m?wmcontentbitrate=300000", "DR Folk er farverig og afvekslende musik, der spænder fra dansk og nordisk folkrock til songwriters og folkemusik fra de britiske øer og Nordamerika.<p>DR Folk har sine rødder i den vestlige folkemusik, og rummer hele den spændende udvikling fra 60’erne revival til de helt aktuelle udgivelser." ),
    new Station( "DR Hiphop", "http://wmscr1.dr.dk/e02ch08m?wmcontentbitrate=300000", "DR Hip Hop spiller det bedste inden for genren - fra undergrund til mainstream. Med vægten på det nyeste fra dansk og U.S. Hip Hop, holder vi dig konstant opdateret og supplerer med de største klassikere gennem tiderne. Det hele blandet godt op med afstikkere til U.K., norden og resten af verden. Og så er det uden snak - kun det pureste Hip Hop...24/7!" ),
    new Station( "DR Hit", "http://wmscr1.dr.dk/e04ch10m?wmcontentbitrate=300000", "DR Hit er blød popmusik med masser af gode hits og nyheder på hele timer.<p>DR Hit - Soundtracket til en lidt bedre dag." ),
    new Station( "DR Jazz", "http://wmscr1.dr.dk/e02ch05m?wmcontentbitrate=300000", "DR Jazz er kanalen der kun sender jazzmusik - 24 timer i døgnet på DAB-radio og net.<p>DR Jazz favner bredt. Væsentligste kriterium er kvalitet med det bedste fra alle jazzens genrer og perioder; og selvfølgelig med særlig opmærksomhed på den danske jazz og DR Big Band.<p>Det er i mikset af musikken, vi skiller os ud fra andre jazzkanaler. Vi spiller det, du gerne vil høre, og så sørger vi også for, at du bliver glædelig overrasket og møder nye kunstnere og konstellationer, du ikke vidste, at du kunne lide eller kendte i forvejen.<p>DR Jazz genudsender også jazzredaktionens udsendelser fra DR P2" ),
    new Station( "DR Klassisk", "http://wmscr1.dr.dk/e02ch06m?wmcontentbitrate=300000", "DR Klassisk på DAB, kabel og net er kanalen for den klassiske musikelsker.<p>DR Klassisk sender hele klassiske værker og hele koncerter - mest live-optagelser, nye eller fra arkivet. Musikken bliver kort og kompetent præsenteret af vore faste værter.<p>På DR Klassisk har den klassiske musik sit fulde udfoldelsesrum og afbrydes ikke af nyheder eller taleindslag.<p>DR Klassisk er dagen igennem det klassiske alternativ til P2, men vi genudsender også de flotte aften koncerter fra P2 - bl.a. torsdagskoncerterne. En mulighed for at høre eller genhøre.<p>DR Klassisk er den rene klassiske kanal på musikkens præmisser." ),
    new Station( "DR Modern Rock", "http://wmscr2.dr.dk/e06ch02m?wmcontentbitrate=300000", "DR Modern Rock er kanalen for dig, hvis rockmusikken bare ikke kan blive nok udadvendt, tempofyldt, varieret, kantet, grænsesøgende, dynamisk og gennemslagskraftig.<p>Kanalen favner både genrens nyeste, hotteste og mest aktuelle hits fra ind- og udland, men er også stedet hvor du møder undergrundens myriader af unge, talentfulde - især danske - bands." ),
    new Station( "DR Nyheder", "http://wmscr2.dr.dk/e04ch11m?wmcontentbitrate=300000", "Lyt til nyheder, når du har lyst.  Hør de  seneste nyheder - opdateret hver time - døgnet rundt. " ),
    new Station( "DR Oline", "http://wmscr1.dr.dk/e04ch12m?wmcontentbitrate=300000", "Oline er radio for børn mellem 3-7 år. Klokken 7.00 og kl. 18.30 er der nyt på Oline hver dag.<p>Oline indeholder masser af historier og det er alt fra gamle klassiske eventyr til børnebøger der lige er udkommet.<p>Der er masser af børn der fortæller om stor og små ting i deres dagligdag - venskaber kæledyr og den nye cykel Sofus fik i fødselsdagsgave - og det er også på Oline du kan høre Anna og Lotte, Sigurds Bjørneradio og Kaj og Andrea i radioversionen.<p>Og så er der masser af musik for lige netop denne målgruppe på Oline. Her finder du den musik som er skrevet til børn - men også engang imellem musik som får en til at spærre ørerne op og blive overrasket. " ),
    new Station( "DR P5000", "http://wmscr2.dr.dk/e06ch04m?wmcontentbitrate=300000", "P5000 er Mascha Vang, Anders Stjernholm og Anders Bonde. Blondinen, festaben og gammelsmølfen er holdet fra helvede. Mixet med sladdernyt og musikken du elsker.<p>Fredag og lørdag skruer vi ekstra op for festen, med top 25 på dancecharten plus to timer mixet af superstar-dj Morten Breum." ),
    new Station( "DR Pop DK", "http://wmscr1.dr.dk/e02ch10m?wmcontentbitrate=300000", "" ),
    new Station( "DR R&B", "http://wmscr1.dr.dk/e06ch08m?wmcontentbitrate=300000", "Er du til bløde toner, så spiller DR R&B musik for dig, der er til R&B og soulmusik. Kanalen indeholder overvejende ny musik, hvilket betyder, at det er her, du kan lytte til de nye hits, og hvad der måtte være på vej.<p>På kanalen kan du høre flere nye numre fra nye udgivelser - og ikke kun én single ad gangen. DR R&B er også klassikere inden for genren og ældre soulhits. Det er også her, du kan tjekke specielle remixes, white labels og meget mere. " ),
    new Station( "DR Rock", "http://wmscr1.dr.dk/e02ch04m?wmcontentbitrate=300000", "DR Rock er rock uden stop - 24 timer i døgnet på DAB og nettet.<p>DR Rock er en rendyrket rockkanal med fokus på den klassiske rock fra 60'erne til i dag. Musikken er et mix af de store klassikere til de bedste albumtracks.<p>DR Rock er kanalen for alle rockelskere. De forskellige genrer inden for den populære rock og med de kunstnere der har defineret og ændret rocken gennem tiderne." ),
    new Station( "DR Soft", "http://wmscr1.dr.dk/e06ch05m?wmcontentbitrate=300000", "DR Soft er til dig, der elsker en god popmelodi.<p>På DR Soft kan du både høre de nyeste hits og de gode gamle popklassikere fra stjerner som Phil Collins, George Michael og Eurythmics.<p>25 procent af den spillede musik er dansk, og der er masser af variation på playlisten.<p>Skriv til redaktionen:<p>Att: DR Soft<br>DR Musik og Medier<br>DR Byen<br>Emil Holms Kanal 20<br>0999 København C<br>Mail: dr-soft@dr.dk" ),
    new Station( "DR Spillemand", "http://wmscr1.dr.dk/e04ch11m?wmcontentbitrate=300000", "DR Spillemand er farverig dansk, nordisk og irsk/skotsk folkemusik som den lyder i dag - spillet og sunget af musikere, der tager afsæt i gamle traditioner.<p>Rødder, fornyelse og nysgerrighed er kerneordene for kanalen, som især fokuserer på akustisk, nordisk musik. " ),
    new Station( "DR World", "http://wmscr1.dr.dk/e06ch12m?wmcontentbitrate=300000", "DR World tilbyder som eneste danske radiokanal verdensmusik døgnet rundt. Her kan du høre det nyeste fra 'World Music Charts Europe' krydret med klassikerne fra alle verdensdele - det velkendte og det ukendte.<p>DR World afspejler storbykulturen og tager udgangspunkt i moderne fortolkning af traditionel musik fra hele kloden, inklusive den danske verdensmusikscene. Hovedparten af musikken, du hører her, er udgivet indenfor de seneste 10 år.<p>DR World er med dig - døgnet rundt - verden rundt." )
);

categories["Diverse"]= new Array (
    new Station( "Radio 2", "http://dix.media.webpartner.dk/radio2-96", "" ),
    new Station( "The Voice", "http://dix.media.webpartner.dk/voice128", "" ),
    new Station( "Radio 100FM", "http://onair.100fmlive.dk/100fm_live.mp3", "" ),
    new Station( "Radio 100FM Soft", "http://onair.100fmlive.dk/soft_live.mp3", "" ),
    new Station( "Cool FM", "http://stream2.coolfm.dk:80/CoolFM128", "" ),
    new Station( "ANR Hit FM2", "mms://media.xstream.dk/Radio_Hit_FM", "" ),
    new Station( "ANR Guld FM", "mms://media.xstream.dk/Radio_Guld_FM", "" ),
    new Station( "Radio ABC", "mms://media.xstream.dk/Radio_ABC", "" ),
    new Station( "ABC Solo FM", "mms://media.xstream.dk/Radio_Solo_FM ", "" ),
    new Station( "Radio Alfa Østjylland", "mms://media.xstream.dk/Radio_Alfa", "" ),
    new Station( "Radio Skive", "mms://media.xstream.dk/Radio_Skive", "" ),
    new Station( "Radio Mojn", "mms://media.xstream.dk/Radio_Mojn", "" ),
    new Station( "Radio 3", "mms://media.xstream.dk/Radio_3", "" ),
    new Station( "Radio Sydhavsøerne", "mms://media.xstream.dk/Radio_Sydhavsoerene", "" ),
    new Station( "NOVAfm", "mms://stream.ventelo.dk/FM5", "" )
);


function DanishStreamsService()
{
    Amarok.debug( "creating Danish Radio Streams service..." );
    ScriptableServiceScript.call( this, "Danish Radio Streams", 2, "List of free online radio stations from Denmark", "List of free online radio stations from Denmark", false );
    Amarok.debug( "done." );
}

function onPopulating( level, callbackData, filter )
{
    if ( level == 1 ) 
    {
        for( att in categories )
        {
            Amarok.debug ("att: " + att + ", " + categories[att].name)
           
            item = Amarok.StreamItem;
	    item.level = 1;
	    item.callbackData = att;
	    item.itemName = att;
	    item.playableUrl = "";
	    item.infoHtml = "";
	    script.insertItem( item );

        }
        script.donePopulating();

    }
    else if ( level == 0 ) 
    {
        Amarok.debug( " Populating station level..." );
	//add the station streams as leaf nodes

        var stationArray = categories[callbackData];

	for ( i = 0; i < stationArray.length; i++ )
	{
		item = Amarok.StreamItem;
		item.level = 0;
		item.callbackData = "";
		item.itemName = stationArray[i].name;
		item.playableUrl = stationArray[i].url;
		item.infoHtml = stationArray[i].description;
                item.artist = "Netradio";
		script.insertItem( item );
	}
	script.donePopulating();
    }
}

script = new DanishStreamsService();
script.populate.connect( onPopulating );
