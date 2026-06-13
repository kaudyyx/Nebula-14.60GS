#pragma once
#include "pch.h"
#include <Engine/Source/ThirdParty/Utils/Utils.h>

enum EBotState : uint8
{
	Warmup,
	InBus,
	SkydivingFromBus,
	Landed,
	Looting,
	MovingAround,
	MovingRandomly,
	MovingToZone,
	MovingAroundUnbreakableObj,
	LookingForPlayers,
	Stuck,
	MAX
};

inline vector<UAthenaCharacterItemDefinition*> CIDs{};
inline vector<UAthenaPickaxeItemDefinition*> Pickaxes{};
inline vector<UAthenaBackpackItemDefinition*> Backpacks{};
inline vector<UAthenaGliderItemDefinition*> Gliders{};
inline vector<UAthenaSkyDiveContrailItemDefinition*> Contrails{};
inline vector<UAthenaDanceItemDefinition*> Dances{};
inline map<AActor*, AFortAthenaAIBotController*> ChestsForBots{};
extern inline int UPTimethingy = 0;
extern inline int stucktime = 0;

struct Bot
{
public:
	AFortAthenaAIBotController* PC = nullptr;
	AFortPlayerPawnAthena* Pawn = nullptr;
	AFortPlayerStateAthena* PlayerState = nullptr;
	bool Emoting = false;
	bool Jumping = false;
	bool Running = false;
	bool ThankedBusDriver = false;
	FVector TargetPOI = FVector();
	AActor* TargetActor = nullptr;
	bool TickEnabled = false;
	float FloatValue = 0.f;
	ABuildingActor* StuckActor = nullptr;
	bool JumpedFromBus = false;
	AActor* TargetGoTo = nullptr;
	FString* BotPlayerName = nullptr;

	EBotState State = EBotState::Warmup;
	EBotState PreviousState = EBotState::MAX;
public:
	void SetName(const FString& NewName)
	{
		auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
		GameMode->ChangeName(PC, NewName, true);

		PlayerState->OnRep_PlayerName();
	}

	FString GetRandomName()
	{
		// Static list of bot names
		static std::vector<std::wstring> botNames = {
L"NomnomCarver",
L"NonCarbon8",
L"NoodleSoup83",
L"NotAPalindrome38",
L"NotDefaultPlayer",
L"NotMyLasagna18",
L"Notatest22",
L"NullAndVoid58",
L"Number141",
L"Numbers789",
L"NumeroDos28",
L"Nuotare_Via",
L"OblongWidget",
L"OddComplex85",
L"Odile2Ray8510",
L"OgniomistrzJan",
L"OldWaterBottle28",
L"OleReliable78",
L"Olliethelandlord",
L"OmeletteDuFromage",
L"OneGlimpse87",
L"OneMillion17",
L"OneMillionAnts24",
L"OnetrickWonder64",
L"OnlyMust4rd",
L"OpenSeason23",
L"OpenShutCase",
L"Ophelia66",
L"Oracle_panda",
L"OrangeTango33",
L"OrganikFasulye",
L"Othello54",
L"Ouboudah25",
L"Overtime46",
L"OxfordCommas",
L"PLZST0P75",
L"PaLyANChO",
L"PaTaTeS_KaFa3",
L"PaccaBoy8000",
L"PanFarfocel",
L"PandinoSuperstar",
L"PanicButton47",
L"Panzabirra98",
L"PapaJimbo9",
L"PapaParadise82",
L"Paracord",
L"ParanoidCactus",
L"ParmesanPeyniri1",
L"ParmesanPeyniri24147",
L"Patapouf_Boss",
L"PatatasHazard",
L"PatatasScope",
L"PatatinoSalvahe",
L"PebbleSpider",
L"PeelyDeservedBetter",
L"Pembe_Örümcek",
L"PepperTaxi",
L"PerpleXed79",
L"Pescatore_2",
L"PeskyInfant",
L"PeterUnlustig55089",
L"Petrarca_aveva_ragione",
L"Phorgas",
L"PickaxeUAQ",
L"Pickles60",
L"PierluigiCoiSoldi",
L"Pinnacle6",
L"Pippettoloscherzetto",
L"PirateWizard22",
L"PizzaKetchupAnanas",
L"PizzaUnic0rn",
L"PlasticBullets62",
L"PlatinumPony",
L"PlayOnRepeat",
L"Playmaker365",
L"PlzBeNiceToMe",
L"PoachedPepper",
L"PopNLock7",
L"PopcornSalt3",
L"PortaSniper",
L"PortableOx",
L"PositiveFeels",
L"PotatoCupid",
L"PotsNPans39",
L"Poule_d0",
L"PowerFlank",
L"Pr0K1ller07",
L"Pr0K1ller08",
L"PrancingPwnee",
L"PrimeSystem",
L"Primefall44",
L"Primetime71",
L"PrinceWombat",
L"PrincessImp",
L"Principessa9832888",
L"Prisoner19A",
L"ProdigioLatino",
L"Przemek2302",
L"PsyonicOx",
L"PumpkinKnight83",
L"PuppyDance9",
L"PureBacon94",
L"PureSneak92",
L"PurpleCrayon85",
L"PurpleRocks99",
L"QWERTYb0ss",
L"Quack4Bread",
L"QuadAlex13",
L"QuantumFig",
L"QuartetUltra",
L"QueenBeet74",
L"R00kMine6",
L"R0ckStar5",
L"RabbitKicks",
L"Racoon808",
L"RafaleNextGen",
L"RagePower81",
L"Railatan65466",
L"RainbowZebra36",
L"Rainmaker67",
L"RamarroMarrone11528",
L"Ramp4ge71",
L"Ramsbottle",
L"RandoDog73",
L"RandomTyp28162",
L"RareOrange",
L"RatherBFishin0",
L"RavensRevenge13",
L"Reach4TheSky54",
L"RealMisfortune",
L"Reaper1487",
L"RedFox ϟ",
L"RedOrange2240",
L"ReginaldoMaccaroni",
L"Reinedugaming65094",
L"RelaxOkay",
L"Renegade53",
L"Represent82",
L"Reyloveslillyann",
L"Rhett5408",
L"Riboflavin52",
L"Riftmage54",
L"Riinos72451",
L"RiskyDigits",
L"RoarTiger24",
L"Robak1lls",
L"RocketBoi87",
L"Rocking7614",
L"Roundhouse47",
L"RoyaleCorral",
L"RoyaleKing60953",
L"Rrrrumba98",
L"RudeAwaken1ngz",
L"RumbayCumbia",
L"RustyShine93",
L"RzeliPapom",
L"SLUGshot65",
L"SMGsRdaBest",
L"SPAL_2019",
L"SaberPhoenix75",
L"Sabretooth67",
L"SadTroll94",
L"Salamander8910",
L"SalemSquid",
L"Salocinu",
L"SalsaPants36",
L"SalsaVerde50",
L"SaltySandwich81",
L"SandSpider2000",
L"Sandra6000",
L"Sandwitch90",
L"Sapphire59",
L"SappyPirate",
L"Sarabat92790",
L"Sarı_Kavun",
L"SassyJazzhands",
L"SaucisseMarteau",
L"ScaltritiSuperScaltro",
L"Schabolub",
L"Schali4Lyfe",
L"SchockerZocker01",
L"Scorpio62",
L"ScorpionOne50",
L"Scream4Poutine",
L"ScrubbyRandall",
L"SeagullTamer",
L"Seba129107",
L"SeeUFriday",
L"SeguraMeuHeadshot",
L"SergeantSummer",
L"SeriousSteve35",
L"Sevarog3",
L"Seven8Nine93",
L"SeñorFiesta60631",
L"Shadow74739",
L"ShadowArrow58",
L"Sharkboy61",
L"Shepard52",
L"ShieldHorse63",
L"ShiniestBovine",
L"ShoTTy4Life48",
L"ShogunGiraffe",
L"Shoot2Thrill25",
L"ShootyMcGee40",
L"Shotdrop87",
L"Shotty2thebody66",
L"ShottyBOI1",
L"ShrimpHeaven74",
L"Shrimpz62",
L"SickPapaya",
L"SilentBanana17",
L"SilentRogue98",
L"SilverBear89",
L"SilverySliver",
L"SimoneSpakky",
L"SimplyKraken",
L"SimplyTheBest97",
L"SirChimera83",
L"SirSkillaLot10694",
L"SirTricksALot21",
L"SixesAndSevens65",
L"SizzlinChamp",
L"SkarpetMistrz",
L"SkeletonMonkey14",
L"Skelettosaure5",
L"Skyfire85",
L"Slammie9",
L"SleepyPete56",
L"SlickElephant",
L"SlothLife16",
L"SlurpItUp93",
L"SlurpMaster650",
L"Sm4tt0",
L"Smellwonk",
L"SmittenKitten4",
L"Smoothie66",
L"SmugglerPL",
L"Sn00tyMagician",
L"Sn1pzChugsmilk",
L"SnappedBinkie71",
L"SneakyBush9898",
L"Sniper36094",
L"SniperCharlie92",
L"SnipersDelight50",
L"SniperzX18623",
L"Snowday95",
L"SoFastMan",
L"SoSweaty63",
L"SoggyCookie26",
L"SoloSpy86",
L"Solocamper8",
L"SomethingFishy3",
L"SonicComet37",
L"SoulIntellect",
L"SoulSpecter",
L"Soundthealarm18",
L"Soytupadre87202",
L"Soytuprofe34783",
L"SpaceChimp18",
L"SpaceLight13",
L"SpaceRivers",
L"SpaghettiCode77",
L"SpaghettiSquad25356",
L"SpectrumKnight13",
L"SpeedSparrow47",
L"SpeedyTheCat98",
L"SpiderMitten",
L"Splut16159",
L"SpokenWard",
L"SpookyLeader21",
L"SporkMaster20",
L"SpringChicken10",
L"SquadMaster22",
L"SquadUp98",
L"Squar3R00t5",
L"StaleGrapes",
L"StarWarden80",
L"StarryGolds",
L"StayHydrated41",
L"Steeeve89002",
L"SteelGoose18",
L"Stellardente",
L"FlavorCapitan",
L"FlawlessGame",
L"FlimsyGoat",
L"FlossPatrol82",
L"Fluffy12229",
L"FluffyMuffin8",
L"FluffySardine",
L"Flyingfender",
L"FootBarn",
L"FootballDay",
L"Forbidden60",
L"ForeverCacciucco",
L"Forge62018",
L"ForkAndKnife17",
L"ForkSpoonKnife42",
L"ForksNSporks",
L"ForlornMorning",
L"ForsakenVoyage",
L"FortGoodNite43",
L"Fortifiable",
L"FortniteKid34558",
L"Frediculous20",
L"FreeCookies",
L"FreeMinded49",
L"Freefire63",
L"FreshFish71",
L"Freylis21",
L"FriendlySniper27",
L"FrogSpawn42",
L"FrogTheCat18",
L"FromageSauvage15026",
L"Frostbitten86",
L"FroyoLicker",
L"Frutilla_80",
L"FunSized86",
L"FunkLogic17",
L"FunkyFresh1349",
L"FuriousBug50",
L"FusilliTime",
L"FuzzyPickl3s",
L"Fußballfreak2003",
L"G3rard0_313",
L"Gaargod55",
L"GalaxyBrain77",
L"GameWinnerz87",
L"GamerGreat30",
L"Gandolfoilbianco",
L"Gangsta_Boi01",
L"GardenDolphin",
L"Gatordile81",
L"Gearhead46",
L"Geckolord1234",
L"GeoForts",
L"Gerard2par289828",
L"GerbilTrapeze",
L"GermoglioDiSole",
L"GetScoped92",
L"GhostChicken12",
L"GhostColour6",
L"GiaPuppy",
L"GiantDaisy",
L"GigaWatt10k",
L"Gigabyte51",
L"GimmeCrisps",
L"GimmeMoar",
L"Giuseppina71",
L"GiusyLove212",
L"Gizmoblock",
L"Gkublok13",
L"Glamour67",
L"GlassCannister",
L"GlitterMantis",
L"GloatingEmu",
L"Glopazoid",
L"Gloriainfinita",
L"GlowBeetle41",
L"GnomeOverlord",
L"GnomeRider16",
L"GoblinSelfie",
L"Going4Gold17",
L"GoldenCardamom",
L"GoldenMage12",
L"GoldenManatee6",
L"GoldenOctopus0",
L"Goldie971",
L"GolfVoid",
L"GoneFishin15",
L"GoodDawg13",
L"GoodIRL",
L"GoodRobin6",
L"Gooddoggo80",
L"Goosezilla13",
L"GotHeartburn",
L"GradedOnACurve",
L"GranManzo",
L"Grandma40",
L"GrannyGunner3",
L"GreasyGoal82",
L"GreatGriffin28",
L"Greeble49703",
L"Greedy33",
L"GreenGag52",
L"Gregbibiche7",
L"Gridl0cked",
L"GrimRanger72",
L"GrumbleofPugs6",
L"Guacamoldy7",
L"GuardianHeart4",
L"Guasón-de-la-Noche94",
L"GuessWhat23",
L"GunGoesPewPew38",
L"GunShowTickets",
L"Gurshwunner",
L"Gwen_A_Du",
L"H1pC0c0nut",
L"HAFkotten",
L"HaHaHeeHee73",
L"HaaalpMe",
L"HakerBonzo66831",
L"HalfEatenPizza82",
L"HamsterTaxi",
L"HandsomeLemon34",
L"HangryBuddy",
L"Happy2Bhere39",
L"HappyMelon50",
L"HashTagonistic",
L"Hashbrown31",
L"HashtagToad57",
L"Haszczak32",
L"HawkWrath74",
L"HazardousPaste",
L"HeavyGionny",
L"Heavygatto",
L"Hekka_tombe",
L"HeliumHog",
L"Helix1_1",
L"HelloMyDude54",
L"HelloPapaguena",
L"Here4Dancing",
L"HermitCrab26",
L"HeroicHealer71",
L"Herz-und-Seele",
L"Hevymeddle",
L"HeyThereFriend81",
L"HippoMagician",
L"Hiveminder97",
L"HogHog28",
L"HollowBrick5",
L"Hollywood70",
L"HomeSkillet95",
L"Hoodwinked12",
L"HoorayBeets",
L"HopOnDown20",
L"HoraCurka53891",
L"Horse4227",
L"HotelBlankets",
L"HowAreMy90s",
L"Howitzer98",
L"HubcapDonuts",
L"Hydrohawk7",
L"HyenaBrigade",
L"IAmW4t3r",
L"IGotSticks85",
L"IGottaGoToBed",
L"IHateBacon35",
L"IHaveFullBrick",
L"IHaveFullMetal",
L"ISeeCiabatta",
L"IamPizza5",
L"IceFork43",
L"IchKannNichLesen",
L"Ichthys25",
L"IckyWombat",
L"IhaveAGoodElo",
L"ImFam0us24",
L"ImGonnaWin2234",
L"ImReallyADog76",
L"ImmortalMango66",
L"Inaffidabile222",
L"InfinityPigeon66",
L"InflatedCouch",
L"Initialize0",
L"InnoScent24",
L"InsideAgent",
L"IntoTheOcean",
L"InverseUnicorn95",
L"IronBoss86",
L"IronHype58",
L"IronMade20",
L"Irontide57",
L"IrritatedIris",
L"IsBruceHome",
L"IsOnlyGame12",
L"IvanDude0001",
L"J005ek1",
L"JackalViper",
L"Jacqueline_die_Queen",
L"JaiFaimJaiSoif",
L"JakeTheSnake7770",
L"JanSerce67",
L"JasioKlucha",
L"JasonPWright",
L"JebidiahSparks",
L"JellyBeats81",
L"JestemHardkorem31515",
L"JewelOfWisdom",
L"JimmyIsOnFire97",
L"Joelemarsouin",
L"JokeFountain",
L"Jokeman258",
L"Joker8926",
L"JollyGoat46",
L"JonesyForever35",
L"JonesyWannabe46",
L"JonnyTheKid52",
L"JoyRJ6420",
L"JoyxxxRJ",
L"João_SP",
L"JumpyMcJump64",
L"JungleProwler25",
L"JustABitEpic",
L"JustAnotherPro42",
L"JustBuster73",
L"JustSugar85",
L"JusteUnBrieDoux28266",
L"Juxtapose48",
L"KafadanHeadshot1",
L"KarpZMilicza",
L"Kartoffel_skillz",
L"KatKat200239",
L"Katil Kene01",
L"KeepScrolling95",
L"KenCrunch",
L"Keviiinn075",
L"KeyFlamingo",
L"KeyboardKing32",
L"KimchiFighter13",
L"KingFlexor",
L"KingHitttt222",
L"KingKevin01234",
L"KingKoala82",
L"KingOfFudge",
L"KingToucan2",
L"KittyCat80",
L"Klu3ツ",
L"Knowaguy",
L"Kokoreç_Kafa",
L"KowalskiDariusz2",
L"Kragoth93",
L"KralKobra12312433",
L"KralKobra1235",
L"Krasser_Babo",
L"Kregore73",
L"KroissantFatal",
L"Krzychhh",
L"Käsestulle27",
L"Kıllı ÖrümJack",
L"L4mboZ3ta",
L"LaCorsaDellaFoglia",
L"LacasadeBangkok",
L"LadyLuck75",
L"LadyMarinaite",
L"LadyValentina79947",
L"LaidBackBeats",
L"Lallamaquellama1",
L"Lallamaquellama2",
L"LambFury",
L"Landsnail80",
L"Lapinou65194",
L"LarsTheKid25",
L"Laserfrog44",
L"LavaHound68",
L"LazyLlama7",
L"LeTueurDesBois7",
L"LeetGamer87",
L"LegolasAmoreMio",
L"LeonLP78511",
L"Leoz1n61577",
L"LetsBePals23",
L"LewtGoblin7",
L"Lexington90",
L"Lh@amaLhoc@",
L"Liftoff36",
L"&darkBeast&",
L"007Витек",
L"0mn1v0r378",
L"1337Sk1lz",
L"1Forest120",
L"1eyedInvent0r",
L"1Такой",
L"240KMH",
L"25_wütende_Affen",
L"2Xtrouble13",
L"2joãozinho_bh2",
L"2sweaty23",
L"360NoScoper29718",
L"360walkaway",
L"404Zurco",
L"42Wow47",
L"44Sedano44",
L"4Regionals",
L"4tnite4ever15",
L"5HoleSniper",
L"5boysindahouse",
L"5p4cc07u770",
L"688.Hitman",
L"8BallWasHere",
L"9000Hippos",
L"90cranker7",
L"90sRunner",
L"9Яков",
L"A Sweaty Dog",
L"ADudeNamedPhil",
L"AGenuineHuman",
L"AGoodOmen",
L"ALLCAPS67",
L"APeelySmoothie",
L"A_Juke9",
L"Abaddon74",
L"AbraKadaver01",
L"ActivateFail",
L"Afuegonski71563",
L"AggroHamster59",
L"Agoraphone-8",
L"Agua_Cat",
L"Ahat4aMouse",
L"Aim Like Idaho",
L"AlbaThor300",
L"Albacore6",
L"AleXandre2k1",
L"AligatorPear",
L"AllTheCandy",
L"Allsmyles:D",
L"AlmaPura95",
L"AlpacaDaSorte",
L"AlphaPlayer34517",
L"Alternative74",
L"Alvaritooooh",
L"AlwaysOnTime38",
L"Amanda9938643",
L"Ambrosia0",
L"AmpleBlindspot",
L"AngelEye30",
L"Angrification",
L"AngryDuck51",
L"AngryMiau",
L"AngryRabbit9",
L"AngryRoommate10",
L"Anonymutt",
L"AplastaBravas",
L"Aplastafritas",
L"Apricot661",
L"Archibard11",
L"Arclite28",
L"ArcticAndroid",
L"Arczi80408",
L"Argathus",
L"Armstrong78",
L"Arrigoni347",
L"Asi_Portakal",
L"AtTheBeach321",
L"AthenaOrApollo",
L"Atomizer5",
L"AttackOLantern",
L"Avlry",
L"AyeBall23",
L"Ayıdoğan_19",
L"BLTZ_Edgadot",
L"BONES_GT4-42",
L"BabySeahorse81",
L"Back2Work61",
L"BadKahuna94",
L"Baethal",
L"Bag0Hammers",
L"BagelBoy82",
L"Bagigio_99",
L"BaguettePower75",
L"BaguetteWarrior11485",
L"Balefire81",
L"BananaPastrami",
L"Banandana82",
L"Barktooth81",
L"Bartolone811",
L"BatTacos",
L"BattleHammer7",
L"BeRooted",
L"BearPuns55",
L"BeeHive7",
L"Beebitme",
L"BeefSupreme35",
L"BeepGoesBeep",
L"BehindYou60",
L"Belltop",
L"BellyFlop40",
L"Beneventoale",
L"BertIsNice",
L"BestDoggo83",
L"BestRegards81",
L"Best_Llama_Ever",
L"BetaChampion",
L"BewareTheGOAT",
L"BigBuddy15",
L"BillyBrilliant23",
L"BillysMom17",
L"BioDragoon",
L"BitOJoy",
L"BitterSecret",
L"BlackCat61",
L"Blackboy03",
L"Blackjack31",
L"BlackkDW",
L"Blastan000000bz",
L"BlaukrautbleibtBlaukraut",
L"BlindCat47",
L"BlinkImGone44",
L"BlipBlap53",
L"Blizzardbeard77",
L"Blueberry1002",
L"BoatRider10",
L"BoatingIsLife",
L"BobDobaleena",
L"BobsUrUncle71",
L"Bobtimus",
L"Bohun15",
L"BoldPrediction",
L"Bongodrum010",
L"BonyHumans",
L"BoogieKing85",
L"Boomstick59",
L"BootThisUp256",
L"BootifulSnow",
L"BorisBqn07",
L"BorisTheGreat37",
L"BossBoi20",
L"Boss_princess_10",
L"BotheredBud1",
L"Bottellonssio89",
L"Botterino55",
L"BowenCactus",
L"BoxOfGoblins",
L"BraddleRoyale80",
L"BrainInvader",
L"Brainshocked0",
L"Brajanek61598",
L"BrandNew47",
L"BraunyBanana",
L"BravasHazard",
L"BreadSandwich42",
L"BreakerBreaker58",
L"BridgeBuilder87",
L"Bro2Max",
L"BroSincerely",
L"BrokenRecord26",
L"Brometheus52",
L"Bronson66",
L"Bronwyn82199",
L"Brotato3131313",
L"Brummibernd",
L"Brun01567996",
L"BrutalNoodle91",
L"Brutalis42",
L"BubbleDragons",
L"BubbleHunter22",
L"Bubbles41",
L"BubblyFalcon",
L"BuddyGuy9518",
L"Buffering5",
L"BumbleB3ar",
L"Bundledbats",
L"Buongiorno Rega",
L"BurgerGuy1011",
L"BushCamper2477",
L"BusinessJoe",
L"ButchSquire",
L"Butinator6582",
L"ButterKid24",
L"ButternutQuash",
L"C O L F Z Y",
L"C-Block54",
L"CGCMcasone",
L"CKiLeLionMtnt",
L"CactusDad80",
L"CafeDave58",
L"Camion4493015",
L"CandyPlumber",
L"Cannonball86",
L"CannotDo42",
L"CantHearYou3",
L"CantTouchMe67",
L"CapivaraCuritibana",
L"CaplassTigreBianca38287",
L"CaptainKiernan",
L"CaptainSakapusse",
L"Carcrusher94",
L"CarnavalPop",
L"CarneAsideOf",
L"CarpMedley",
L"Carroarmato92",
L"Caspian79",
L"Cathode57",
L"CatsClause",
L"Caveman59",
L"Centice49",
L"Ces87Lasagna",
L"Ch0cOpaup",
L"Ch0pp3r66",
L"ChArter52",
L"Channel2659",
L"CharmedImSure85",
L"Chaseml2012",
L"Cheese10128",
L"Chef86413",
L"CherubTree",
L"ChestOfFault",
L"Chickaboom64324",
L"ChickenFarmer17",
L"ChiliDog87",
L"ChillMono",
L"Chimuelopr0",
L"Chipper70",
L"Chorizo1337",
L"Chronicled5",
L"Chronolink24",
L"Chug_Brigade",
L"Cihy_PL",
L"ClaudiusCamillus",
L"Cleopatra30",
L"CleverPanda91",
L"CleverPun92",
L"CliffyPrawn2431",
L"CocoMarySRL",
L"CoffeeFueled84",
L"ColonelChugJug",
L"ColonelFox41",
L"ColorfulCoyote",
L"ComingForYou19",
L"ConstantThorn",
L"CopperBottom43",
L"CopyKat58",
L"Corrosion86",
L"Cory jones7897",
L"CounterMantra",
L"CovertClown95",
L"CowJuice92",
L"CrabInMyShoe",
L"CrabbyBaby71",
L"CrazyAlpaca98",
L"CrazyJet83",
L"CrazyPea96",
L"CrepeSalad",
L"Crestfallen64",
L"CrocodileBard",
L"CruiseOn",
L"CrumbBum91",
L"Crypt1c60",
L"CryptoDancer",
L"CtrlAltDeleted94",
L"Curveball20",
L"CyberWolf26",
L"CyborgMelon",
L"CyborgWalrus6",
L"D0mb1l1P4nd4",
L"D34THBR1NG3R12",
L"D3m0nk3y56",
L"DJHoneybun",
L"DJLightning85",
L"Damarus83",
L"DanIsHere50",
L"DangerLeopard",
L"DangerZero32",
L"DanglingKeys",
L"DarioAvaro",
L"DarkTarget12",
L"DarlingSnail",
L"Dawson201",
		};

		if (botNames.empty())
		{
			return FString(TEXT("NoMoreNames"));
		}


		static std::random_device rd;
		static std::mt19937 gen(rd());
		std::uniform_int_distribution<> dis(0, botNames.size() - 1);

		TargetActor = FindNearestBuildingSMActor();

		int index = dis(gen);
		std::wstring selectedName = botNames[index];
		botNames.erase(botNames.begin() + index);

		// Convert to FString and return
		FString NewName(selectedName.c_str());
		return NewName;
	}

	void GiveRandomAiLoot() {


		TArray<UFortItemDefinition*> AiLoot;


		AiLoot.Add(Utils::StaticLoadObject<UFortItemDefinition>("/Game/Athena/Items/Weapons/WID_Assault_Surgical_Athena_R_Ore_T03.WID_Assault_Surgical_Athena_R_Ore_T03"));
		AiLoot.Add(Utils::StaticLoadObject<UFortItemDefinition>("/Game/Athena/Items/Weapons/WID_Assault_Surgical_Athena_UC_Ore_T03.WID_Assault_Surgical_Athena_UC_Ore_T03"));
		AiLoot.Add(Utils::StaticLoadObject<UFortItemDefinition>("/Game/Athena/Items/Weapons/WID_Assault_Surgical_Athena_VR_Ore_T03.WID_Assault_Surgical_Athena_VR_Ore_T03"));
		AiLoot.Add(Utils::StaticLoadObject<UFortItemDefinition>("/Game/Athena/Items/Weapons/WID_Assault_Surgical_Athena_SR_Ore_T03.WID_Assault_Surgical_Athena_SR_Ore_T03"));
		AiLoot.Add(Utils::StaticLoadObject<UFortItemDefinition>("/Game/Athena/Items/Weapons/WID_Assault_SemiAuto_Athena_UC_Ore_T03.WID_Assault_SemiAuto_Athena_UC_Ore_T03"));
		AiLoot.Add(Utils::StaticLoadObject<UFortItemDefinition>("/Game/Athena/Items/Weapons/WID_Assault_SemiAuto_Athena_C_Ore_T03.WID_Assault_SemiAuto_Athena_C_Ore_T03"));
		AiLoot.Add(Utils::StaticLoadObject<UFortItemDefinition>("/Game/Athena/Items/Weapons/WID_Assault_SemiAuto_Athena_R_Ore_T03.WID_Assault_SemiAuto_Athena_R_Ore_T03"));
		AiLoot.Add(Utils::StaticLoadObject<UFortItemDefinition>("/Game/Athena/Items/Weapons/WID_Assault_SemiAuto_Athena_VR_Ore_T03.WID_Assault_SemiAuto_Athena_VR_Ore_T03"));
		AiLoot.Add(Utils::StaticLoadObject<UFortItemDefinition>("/Game/Athena/Items/Weapons/WID_Assault_SemiAuto_Athena_SR_Ore_T03.WID_Assault_SemiAuto_Athena_SR_Ore_T03"));
		AiLoot.Add(Utils::StaticLoadObject<UFortItemDefinition>("/Game/Athena/Items/Weapons/WID_Assault_Auto_Athena_UC_Ore_T03.WID_Assault_Auto_Athena_UC_Ore_T03"));
		AiLoot.Add(Utils::StaticLoadObject<UFortItemDefinition>("/Game/Athena/Items/Weapons/WID_Assault_Auto_Athena_C_Ore_T03.WID_Assault_Auto_Athena_C_Ore_T03"));
		AiLoot.Add(Utils::StaticLoadObject<UFortItemDefinition>("/Game/Athena/Items/Weapons/WID_Assault_Auto_Athena_R_Ore_T03.WID_Assault_Auto_Athena_R_Ore_T03"));
		AiLoot.Add(Utils::StaticLoadObject<UFortItemDefinition>("/Game/Athena/Items/Weapons/WID_Assault_AutoHigh_Athena_SR_Ore_T03.WID_Assault_AutoHigh_Athena_SR_Ore_T03"));
		AiLoot.Add(Utils::StaticLoadObject<UFortItemDefinition>("/Game/Athena/Items/Weapons/WID_Assault_AutoHigh_Athena_VR_Ore_T03.WID_Assault_AutoHigh_Athena_VR_Ore_T03"));
		AiLoot.Add(Utils::StaticLoadObject<UFortItemDefinition>("/HighTower/Items/Tomato/Tomato_Rifle/WID_Assault_Stark_Athena_R_Ore_T03.WID_Assault_Stark_Athena_R_Ore_T03"));
		AiLoot.Add(Utils::StaticLoadObject<UFortItemDefinition>("/HighTower/Items/Tomato/Tomato_Rifle/WID_Assault_Stark_Athena_VR_Ore_T03.WID_Assault_Stark_Athena_VR_Ore_T03"));
		AiLoot.Add(Utils::StaticLoadObject<UFortItemDefinition>("/Game/Athena/Items/Weapons/WID_Shotgun_Charge_Athena_C_Ore_T03.WID_Shotgun_Charge_Athena_C_Ore_T03"));
		AiLoot.Add(Utils::StaticLoadObject<UFortItemDefinition>("/Game/Athena/Items/Weapons/WID_Shotgun_Charge_Athena_R_Ore_T03.WID_Shotgun_Charge_Athena_R_Ore_T03"));
		AiLoot.Add(Utils::StaticLoadObject<UFortItemDefinition>("/Game/Athena/Items/Weapons/WID_Shotgun_Charge_Athena_VR_Ore_T03.WID_Shotgun_Charge_Athena_VR_Ore_T03"));
		AiLoot.Add(Utils::StaticLoadObject<UFortItemDefinition>("/Game/Athena/Items/Weapons/WID_Shotgun_Charge_Athena_SR_Ore_T03.WID_Shotgun_Charge_Athena_SR_Ore_T03"));
		AiLoot.Add(Utils::StaticLoadObject<UFortItemDefinition>("/Game/Athena/Items/Weapons/WID_Shotgun_Standard_Athena_VR_Ore_T03.WID_Shotgun_Standard_Athena_VR_Ore_T03"));
		AiLoot.Add(Utils::StaticLoadObject<UFortItemDefinition>("/Game/Athena/Items/Weapons/WID_Shotgun_Standard_Athena_SR_Ore_T03.WID_Shotgun_Standard_Athena_SR_Ore_T03"));
		AiLoot.Add(Utils::StaticLoadObject<UFortItemDefinition>("/Game/Athena/Items/Weapons/WID_Shotgun_Standard_Athena_R_Ore_T03.WID_Shotgun_Standard_Athena_R_Ore_T03"));
		AiLoot.Add(Utils::StaticLoadObject<UFortItemDefinition>("/Game/Athena/Items/Weapons/WID_Shotgun_Standard_Athena_C_Ore_T03.WID_Shotgun_Standard_Athena_C_Ore_T03"));
		AiLoot.Add(Utils::StaticLoadObject<UFortItemDefinition>("/Game/Athena/Items/Weapons/WID_Shotgun_Combat_Athena_SR_Ore_T03.WID_Shotgun_Combat_Athena_SR_Ore_T03"));
		AiLoot.Add(Utils::StaticLoadObject<UFortItemDefinition>("/Game/Athena/Items/Weapons/WID_Shotgun_Combat_Athena_R_Ore_T03.WID_Shotgun_Combat_Athena_R_Ore_T03"));
		AiLoot.Add(Utils::StaticLoadObject<UFortItemDefinition>("/Game/Athena/Items/Weapons/WID_Shotgun_Combat_Athena_VR_Ore_T03.WID_Shotgun_Combat_Athena_VR_Ore_T03"));
		AiLoot.Add(Utils::StaticLoadObject<UFortItemDefinition>("/Game/Athena/Items/Weapons/WID_Pistol_AutoHeavySuppressed_Athena_UC_Ore_T03.WID_Pistol_AutoHeavySuppressed_Athena_UC_Ore_T03"));
		AiLoot.Add(Utils::StaticLoadObject<UFortItemDefinition>("/Game/Athena/Items/Weapons/WID_Pistol_AutoHeavySuppressed_Athena_C_Ore_T03.WID_Pistol_AutoHeavySuppressed_Athena_C_Ore_T03"));
		AiLoot.Add(Utils::StaticLoadObject<UFortItemDefinition>("/Game/Athena/Items/Weapons/WID_Pistol_AutoHeavySuppressed_Athena_R_Ore_T03.WID_Pistol_AutoHeavySuppressed_Athena_R_Ore_T03"));
		AiLoot.Add(Utils::StaticLoadObject<UFortItemDefinition>("/Game/Athena/Items/Weapons/WID_Pistol_Scavenger_Athena_UC_Ore_T03.WID_Pistol_Scavenger_Athena_UC_Ore_T03"));
		AiLoot.Add(Utils::StaticLoadObject<UFortItemDefinition>("/Game/Athena/Items/Weapons/WID_Pistol_Scavenger_Athena_C_Ore_T03.WID_Pistol_Scavenger_Athena_C_Ore_T03"));
		AiLoot.Add(Utils::StaticLoadObject<UFortItemDefinition>("/Game/Athena/Items/Weapons/WID_Pistol_Scavenger_Athena_R_Ore_T03.WID_Pistol_Scavenger_Athena_R_Ore_T03"));
		AiLoot.Add(Utils::StaticLoadObject<UFortItemDefinition>("/Game/Athena/Items/Weapons/WID_Pistol_Scavenger_Athena_VR_Ore_T03.WID_Pistol_Scavenger_Athena_VR_Ore_T03"));
		AiLoot.Add(Utils::StaticLoadObject<UFortItemDefinition>("/Game/Athena/Items/Weapons/WID_Pistol_Scavenger_Athena_SR_Ore_T03.WID_Pistol_Scavenger_Athena_SR_Ore_T03"));
		AiLoot.Add(Utils::StaticLoadObject<UFortItemDefinition>("/Game/Athena/Items/Weapons/WID_Pistol_SemiAuto_Athena_UC_Ore_T03.WID_Pistol_SemiAuto_Athena_UC_Ore_T03"));
		AiLoot.Add(Utils::StaticLoadObject<UFortItemDefinition>("/Game/Athena/Items/Weapons/WID_Pistol_SemiAuto_Athena_C_Ore_T03.WID_Pistol_SemiAuto_Athena_C_Ore_T03"));
		AiLoot.Add(Utils::StaticLoadObject<UFortItemDefinition>("/Game/Athena/Items/Weapons/WID_Pistol_SemiAuto_Athena_R_Ore_T03.WID_Pistol_SemiAuto_Athena_R_Ore_T03"));
		AiLoot.Add(Utils::StaticLoadObject<UFortItemDefinition>("/Game/Athena/Items/Weapons/WID_Pistol_Sixshooter_Athena_SR_Ore_T03.WID_Pistol_SixShooter_Athena_SR_Ore_T03"));
		AiLoot.Add(Utils::StaticLoadObject<UFortItemDefinition>("/Game/Athena/Items/Weapons/WID_Pistol_Sixshooter_Athena_VR_Ore_T03.WID_Pistol_SixShooter_Athena_VR_Ore_T03"));
		AiLoot.Add(Utils::StaticLoadObject<UFortItemDefinition>("/Game/Athena/Items/Consumables/AppleSun/WID_Athena_AppleSun.WID_Athena_AppleSun"));
		AiLoot.Add(Utils::StaticLoadObject<UFortItemDefinition>("/Game/Athena/Items/Consumables/TowerGrenade/Athena_TowerGrenade.Athena_TowerGrenade"));
		AiLoot.Add(Utils::StaticLoadObject<UFortItemDefinition>("/Game/Athena/Items/Consumables/ShockwaveGrenade/Athena_ShockGrenade.Athena_ShockGrenade"));
		AiLoot.Add(Utils::StaticLoadObject<UFortItemDefinition>("/Game/Athena/Items/Weapons/WID_Sniper_BoltAction_Scope_Athena_R_Ore_T03.WID_Sniper_BoltAction_Scope_Athena_R_Ore_T03"));
		AiLoot.Add(Utils::StaticLoadObject<UFortItemDefinition>("/Game/Athena/Items/Weapons/WID_Sniper_BoltAction_Scope_Athena_VR_Ore_T03.WID_Sniper_BoltAction_Scope_Athena_VR_Ore_T03"));
		AiLoot.Add(Utils::StaticLoadObject<UFortItemDefinition>("/Game/Athena/Items/Weapons/WID_Sniper_BoltAction_Scope_Athena_SR_Ore_T03.WID_Sniper_BoltAction_Scope_Athena_SR_Ore_T03"));
		AiLoot.Add(Utils::StaticLoadObject<UFortItemDefinition>("/HighTower/Items/Tomato/Tomato_Rifle/WID_Assault_Stark_Athena_SR_Ore_T03.WID_Assault_Stark_Athena_SR_Ore_T03"));
		AiLoot.Add(Utils::StaticLoadObject<UFortItemDefinition>("/Game/Athena/Items/Traps/TID_Context_BouncePad_Athena.TID_Context_BouncePad_Athena"));
		AiLoot.Add(Utils::StaticLoadObject<UFortItemDefinition>("/Game/Athena/Items/Consumables/Bucket/WID_Athena_Bucket_Old.WID_Athena_Bucket_Old"));
		AiLoot.Add(Utils::StaticLoadObject<UFortItemDefinition>("/Game/Athena/Items/Consumables/ChillBronco/Athena_ChillBronco.Athena_ChillBronco"));
		AiLoot.Add(Utils::StaticLoadObject<UFortItemDefinition>("/Game/Athena/Items/Consumables/Bandage/Athena_Bandage.Athena_Bandage"));
		AiLoot.Add(Utils::StaticLoadObject<UFortItemDefinition>("/Game/Athena/Items/Consumables/Medkit/Athena_Medkit.Athena_Medkit"));



		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<> distrib(0, AiLoot.Num() - 1);
		int randomIndex = distrib(gen);
		UFortItemDefinition* randomItem = AiLoot[randomIndex];
		int randomQuantity = std::rand() % 5 + 1;
		GiveItem(randomItem, randomQuantity);
	}

	ABuildingSMActor* FindNearestBuildingSMActor()
	{
		return nullptr;
		static TArray<AActor*> Array;
		static bool First = false;
		if (!First)
		{
			First = true;
			UGameplayStatics::GetDefaultObj()->GetAllActorsOfClass(UWorld::GetWorld(), ABuildingSMActor::StaticClass(), &Array);
		}

		AActor* NearestPoi = nullptr;

		for (size_t i = 0; i < Array.Num(); i++)
		{
			if (!NearestPoi || (((ABuildingSMActor*)NearestPoi)->GetHealth() < 1500 && ((ABuildingSMActor*)NearestPoi)->GetHealth() > 1 && Array[i]->GetDistanceTo(Pawn) < NearestPoi->GetDistanceTo(Pawn)))
			{
				NearestPoi = Array[i];
			}
		}

		return (ABuildingSMActor*)NearestPoi;
	}

public:

	void GiveItem(UFortItemDefinition* Def, int Count = 1, int LoadedAmmo = 0)
	{
		if (!PC->Inventory)
			PC->Inventory = Utils::SpawnActor22<AFortInventory>({}, {}, PC);
		UFortWorldItem* Item = (UFortWorldItem*)Def->CreateTemporaryItemInstanceBP(Count, 0);
		Item->OwnerInventory = PC->Inventory;
		Item->ItemEntry.LoadedAmmo = LoadedAmmo;
		PC->Inventory->Inventory.ReplicatedEntries.Add(Item->ItemEntry);
		PC->Inventory->Inventory.ItemInstances.Add(Item);
		PC->Inventory->Inventory.MarkItemDirty(Item->ItemEntry);
		PC->Inventory->HandleInventoryLocalUpdate();
	}

	FFortItemEntry* GetEntry(UFortItemDefinition* Def)
	{
		for (size_t i = 0; i < PC->Inventory->Inventory.ReplicatedEntries.Num(); i++)
		{
			if (PC->Inventory->Inventory.ReplicatedEntries[i].ItemDefinition == Def)
				return &PC->Inventory->Inventory.ReplicatedEntries[i];
		}

		return nullptr;
	}

	Bot(AActor* SpawnLocator)
	{

		if (!Globals::bEnablephoebe)
			return;

		auto CID = CIDs[UKismetMathLibrary::GetDefaultObj()->RandomIntegerInRange(0, CIDs.size() - 1)];
		auto Backpack = Backpacks[UKismetMathLibrary::GetDefaultObj()->RandomIntegerInRange(0, Backpacks.size() - 1)];
		auto Glider = Gliders[UKismetMathLibrary::GetDefaultObj()->RandomIntegerInRange(0, Gliders.size() - 1)];
		auto Contrail = Contrails[UKismetMathLibrary::GetDefaultObj()->RandomIntegerInRange(0, Contrails.size() - 1)];

		if (!CID || !CID->HeroDefinition || !Backpack || !Glider || !Contrail)
			return;

		Pawn = Utils::BotMutator->SpawnBot(Utils::StaticLoadObject<UClass>("/Game/Athena/AI/Phoebe/BP_PlayerPawn_Athena_Phoebe.BP_PlayerPawn_Athena_Phoebe_C"), SpawnLocator, SpawnLocator->K2_GetActorLocation(), SpawnLocator->K2_GetActorRotation(), false);
		PC = (AFortAthenaAIBotController*)Pawn->Controller;
		PlayerState = (AFortPlayerStateAthena*)PC->PlayerState;

		auto PickDef = Pickaxes[UKismetMathLibrary::GetDefaultObj()->RandomIntegerInRange(0, Pickaxes.size() - 1)];
		if (!PickDef)
		{
			TickEnabled = false;
			printf("Null!\n");
			return;
		}

		if (PickDef && PickDef->WeaponDefinition)
		{
			GiveItem(PickDef->WeaponDefinition);
		}

		FString BotNewName = GetRandomName();
		SetName(BotNewName);

		auto Entry = GetEntry(PickDef->WeaponDefinition);
		Pawn->EquipWeaponDefinition((UFortWeaponItemDefinition*)Entry->ItemDefinition, Entry->ItemGuid);

		if (CID->HeroDefinition)
		{
			if (CID->HeroDefinition->Specializations.IsValid())
			{
				for (size_t i = 0; i < CID->HeroDefinition->Specializations.Num(); i++)
				{
					UFortHeroSpecialization* Spec = Utils::StaticLoadObject<UFortHeroSpecialization>(UKismetStringLibrary::GetDefaultObj()->Conv_NameToString(CID->HeroDefinition->Specializations[i].ObjectID.AssetPathName).ToString());
					if (Spec)
					{
						for (size_t j = 0; j < Spec->CharacterParts.Num(); j++)
						{
							UCustomCharacterPart* Part = Utils::StaticLoadObject<UCustomCharacterPart>(UKismetStringLibrary::GetDefaultObj()->Conv_NameToString(Spec->CharacterParts[j].ObjectID.AssetPathName).ToString());
							if (Part)
							{
								PlayerState->CharacterData.Parts[(uintptr_t)Part->CharacterPartType] = Part;
							}
						}
					}
				}
			}
		}

		for (size_t j = 0; j < Backpack->CharacterParts.Num(); j++)
		{
			UCustomCharacterPart* Part = Backpack->CharacterParts[j];
			if (Part)
			{
				PlayerState->CharacterData.Parts[(uintptr_t)Part->CharacterPartType] = Part;
			}
		}

		PC->CosmeticLoadoutBC.Glider = Glider;
		PC->CosmeticLoadoutBC.SkyDiveContrail = Contrail;
		Pawn->CosmeticLoadout = PC->CosmeticLoadoutBC;

		for (auto SkillSet : PC->BotSkillSetClasses)
		{
			if (!SkillSet)
				continue;

			if (auto AimingSkill = Utils::Cast<UFortAthenaAIBotAimingDigestedSkillSet>(SkillSet))
				PC->CacheAimingDigestedSkillSet = AimingSkill;

			if (auto AttackingSkill = Utils::Cast<UFortAthenaAIBotAttackingDigestedSkillSet>(SkillSet))
				PC->CacheAttackingSkillSet = AttackingSkill;

			if (auto HarvestSkill = Utils::Cast<UFortAthenaAIBotHarvestDigestedSkillSet>(SkillSet))
				PC->CacheHarvestDigestedSkillSet = HarvestSkill;

			if (auto InventorySkill = Utils::Cast<UFortAthenaAIBotInventoryDigestedSkillSet>(SkillSet))
				PC->CacheInventoryDigestedSkillSet = InventorySkill;

			if (auto LootingSkill = Utils::Cast<UFortAthenaAIBotLootingDigestedSkillSet>(SkillSet))
				PC->CacheLootingSkillSet = LootingSkill;

			if (auto MovementSkill = Utils::Cast<UFortAthenaAIBotMovementDigestedSkillSet>(SkillSet))
				PC->CacheMovementSkillSet = MovementSkill;

			if (auto PerceptionSkill = Utils::Cast<UFortAthenaAIBotPerceptionDigestedSkillSet>(SkillSet))
				PC->CachePerceptionDigestedSkillSet = PerceptionSkill;

			if (auto PlayStyleSkill = Utils::Cast<UFortAthenaAIBotPlayStyleDigestedSkillSet>(SkillSet))
				PC->CachePlayStyleSkillSet = PlayStyleSkill;
		}


		UBlackboardData* BB = Utils::StaticLoadObject<UBlackboardData>("/Game/Athena/AI/Phoebe/BehaviorTrees/BB_Phoebe.BB_Phoebe");

		PC->PathFollowingComponent->MyNavData = ((UAthenaNavSystem*)UWorld::GetWorld()->NavigationSystem)->MainNavData;
		PC->PathFollowingComponent->OnNavDataRegistered(((UAthenaNavSystem*)UWorld::GetWorld()->NavigationSystem)->MainNavData);

		PC->UseBlackboard(BB, &*(UBlackboardComponent**)(__int64(PC) + Utils::EonGetOffset(PC, "Blackboard")));
		PC->UseBlackboard(BB, &*(UBlackboardComponent**)(__int64(PC) + Utils::EonGetOffset(PC, "Blackboard1")));

		PlayerState->OnRep_CharacterData();

		Pawn->CapsuleComponent->SetGenerateOverlapEvents(true);
		Pawn->CharacterMovement->bCanWalkOffLedges = true;

		FGameplayEffectContextHandle EffectHandle{};
		auto GE = Utils::StaticLoadObject<UClass>("/Game/Athena/Items/Gameplay/BackPacks/Ashton/GE_AshtonPack_FallDamageImmune.GE_AshtonPack_FallDamageImmune_C");
		PlayerState->AbilitySystemComponent->BP_ApplyGameplayEffectToSelf(GE, true, EffectHandle);

		TickEnabled = true;
	}

public:
	/*void Emote()
	{
		if (Emoting)
			return;
		auto EmoteDef = Dances[UKismetMathLibrary::GetDefaultObj()->RandomIntegerInRange(0, Dances.size() - 1)];
		if (!EmoteDef)
			return;
		Emoting = true;
		static UClass* EmoteAbilityClass = Utils::StaticLoadObject<UClass>("/Game/Abilities/Emotes/GAB_Emote_Generic.GAB_Emote_Generic_C");
		FGameplayAbilitySpec Spec{};
		AbilitySpecCtor(&Spec, reinterpret_cast<UGameplayAbility*>(EmoteAbilityClass->DefaultObject), 1, -1, EmoteDef);
		GiveAbilityAndActivateOnce(reinterpret_cast<AFortPlayerStateAthena*>(PC->PlayerState)->AbilitySystemComponent, &Spec.Handle, Spec, nullptr);
	}*/

	ABuildingActor* FindNearestChest()
	{
		static auto ChestClass = Utils::StaticLoadObject<UClass>("/Game/Building/ActorBlueprints/Containers/Tiered_Chest_Athena.Tiered_Chest_Athena_C");
		TArray<AActor*> Array;
		UGameplayStatics::GetDefaultObj()->GetAllActorsOfClass(UWorld::GetWorld(), ChestClass, &Array);
		AActor* NearestPoi = nullptr;

		for (size_t i = 0; i < Array.Num(); i++)
		{
			AActor* Actor = Array[i];
			if (ChestsForBots.contains(Array[i]) && ChestsForBots[Actor] != PC)
				continue;
			if (!NearestPoi || Array[i]->GetDistanceTo(Pawn) < NearestPoi->GetDistanceTo(Pawn))
			{
				NearestPoi = Array[i];
			}
		}
		Array.Free();
		return (ABuildingActor*)NearestPoi;
	}

	AFortPickupAthena* FindNearestPickup()
	{
		static auto ChestClass = AFortPickupAthena::StaticClass();
		TArray<AActor*> Array;
		UGameplayStatics::GetDefaultObj()->GetAllActorsOfClass(UWorld::GetWorld(), ChestClass, &Array);
		AActor* NearestPoi = nullptr;

		for (size_t i = 0; i < Array.Num(); i++)
		{
			if (!NearestPoi || Array[i]->GetDistanceTo(Pawn) < NearestPoi->GetDistanceTo(Pawn))
			{
				NearestPoi = Array[i];
			}
		}
		Array.Free();
		return (AFortPickupAthena*)NearestPoi;
	}

	AFortPlayerPawnAthena* FindNearestPlayer()
	{
		auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;

		AActor* NearestPoi = nullptr;

		for (size_t i = 0; i < GameMode->AlivePlayers.Num(); i++)
		{
			if (!NearestPoi || (GameMode->AlivePlayers.IsValidIndex(i) && GameMode->AlivePlayers[i] && GameMode->AlivePlayers[i]->Pawn && GameMode->AlivePlayers[i]->Pawn->GetDistanceTo(Pawn) < NearestPoi->GetDistanceTo(Pawn)))
			{
				NearestPoi = GameMode->AlivePlayers[i]->Pawn;
			}
		}

		for (size_t i = 0; i < GameMode->AliveBots.Num(); i++)
		{
			if (GameMode->AliveBots.IsValidIndex(i) && GameMode->AliveBots[i] && GameMode->AliveBots[i]->Pawn && GameMode->AliveBots[i]->Pawn != Pawn)
			{
				if (!NearestPoi || GameMode->AliveBots[i]->Pawn->GetDistanceTo(Pawn) < NearestPoi->GetDistanceTo(Pawn))
				{
					NearestPoi = GameMode->AliveBots[i]->Pawn;
				}
			}
		}

		return (AFortPlayerPawnAthena*)NearestPoi;
	}

	void StartLooting()
	{
		State = EBotState::Looting;
		TargetActor = FindNearestChest();
		ChestsForBots[TargetActor] = PC;
	}

	void GoTo(AActor* Loc, float Radius = 0)
	{
		if (TargetGoTo == Loc)
			return;

		TargetGoTo = Loc;
		PC->MoveToActor(Loc, 0, true, false, true, nullptr, true);
		Run();
	}

	void GoTo(FVector Loc, float Radius = 0)
	{
		PC->MoveToLocation(Loc, 0, true, false, false, true, nullptr, true);
	}

	void testfunction(FVector Loc, float Radius = 0)
	{
		PC->K2_GetPawn()->K2_TeleportTo(Loc, 0);
	}

	void WalkForward()
	{
		Pawn->AddMovementInput(Pawn->GetActorForwardVector(), 1.f, true);
	}

	void WalkBackwards()
	{
		Pawn->AddMovementInput(UKismetMathLibrary::GetDefaultObj()->NegateVector(Pawn->GetActorForwardVector()), 1.f, true);
	}

	void WalkLeft()
	{
		Pawn->AddMovementInput(UKismetMathLibrary::GetDefaultObj()->NegateVector(Pawn->GetActorRightVector()), 1.f, true);
	}

	void WalkRight()
	{
		Pawn->AddMovementInput(Pawn->GetActorRightVector(), 1.f, true);
	}

	void Rift()
	{
		bool bRifted = false;
		auto weapondef = Utils::StaticLoadObject<UFortItemDefinition>("/Game/Athena/Items/Consumables/RiftItem/Athena_Rift_Item.Athena_Rift_Item");

		if (!bRifted)
		{
			GiveItem(weapondef, 1, 1);
			bRifted = true;
		}

		auto Entry = GetEntry(weapondef);
		Pawn->EquipWeaponDefinition((UFortWeaponItemDefinition*)Entry->ItemDefinition, Entry->ItemGuid);
		Pawn->PawnStartFire(0);
		EquipPickaxe();
	}

	int GetAimMaxOffset()
	{
		return 300;
	}

	FVector GetAimDirection(AActor* Actor)
	{
		int MaxOffset = GetAimMaxOffset();
		FVector Loc = Actor->K2_GetActorLocation();
		Loc.X += UKismetMathLibrary::GetDefaultObj()->RandomIntegerInRange(-MaxOffset, MaxOffset);
		Loc.Y += UKismetMathLibrary::GetDefaultObj()->RandomIntegerInRange(-MaxOffset, MaxOffset);
		Loc.Z += UKismetMathLibrary::GetDefaultObj()->RandomIntegerInRange(-MaxOffset, MaxOffset);
		return Loc;
	}




	void LookAt(AActor* Actor, bool Bloom = false)
	{
		if (!Actor || !Pawn || PC->GetFocusActor() == Actor)
			return;

		if (!Actor)
		{
			PC->K2_ClearFocus();
			return;
		}

		if (Bloom)
		{
			PC->K2_SetFocalPoint(GetAimDirection(Actor));
		}
		else
		{
			PC->K2_SetFocus(Actor);
		}
	}

	void Skydive(bool FromBus = true)
	{
		Pawn->BeginSkydiving(FromBus);
		Pawn->CurrentMovementStyle = EFortMovementStyle::Running;
		Pawn->CharacterMovement->MovementMode = EMovementMode::MOVE_Custom;
		Pawn->CharacterMovement->CustomMovementMode = 4;
	}

	void Run()
	{
		if (!Running)
		{
			Running = true;
			for (size_t i = 0; i < PlayerState->AbilitySystemComponent->ActivatableAbilities.Items.Num(); i++)
			{
				if (PlayerState->AbilitySystemComponent->ActivatableAbilities.Items[i].Ability->IsA(UFortGameplayAbility_Sprint::StaticClass()))
				{
					PlayerState->AbilitySystemComponent->ServerTryActivateAbility(PlayerState->AbilitySystemComponent->ActivatableAbilities.Items[i].Handle, PlayerState->AbilitySystemComponent->ActivatableAbilities.Items[i].InputPressed, PlayerState->AbilitySystemComponent->ActivatableAbilities.Items[i].ActivationInfo.PredictionKeyWhenActivated);
					break;
				}
			}
		}
	}

	void ForceJump()
	{
		if (JumpedFromBus)
			return;

		Pawn->K2_TeleportTo(((AFortGameStateAthena*)UWorld::GetWorld()->GameState)->Aircrafts[0]->K2_GetActorLocation(), {});
		Skydive();

		TargetPOI = FindNearestChest()->K2_GetActorLocation();
		State = EBotState::Landed;
	}

	FVector GetNearestPoi()
	{
		AActor* NearestPoi = nullptr;

		for (size_t i = 0; i < ((AFortGameStateAthena*)UWorld::GetWorld()->GameState)->PoiManager->AllPoiVolumes.Num(); i++)
		{
			if (!NearestPoi || ((AFortGameStateAthena*)UWorld::GetWorld()->GameState)->PoiManager->AllPoiVolumes[i]->GetDistanceTo(Pawn) < NearestPoi->GetDistanceTo(Pawn))
			{
				NearestPoi = ((AFortGameStateAthena*)UWorld::GetWorld()->GameState)->PoiManager->AllPoiVolumes[i];
			}
		}

		return NearestPoi->K2_GetActorLocation();
	}

	void LookAtReal(const FVector& TargetLocation, bool Bloom = false)
	{

		PC->K2_SetFocalPoint(TargetLocation);

	}

	void EquipPickaxe()
	{
		if (!Pawn || !Pawn->CurrentWeapon)
			return;

		if (!Pawn->CurrentWeapon->WeaponData->IsA(UFortWeaponMeleeItemDefinition::StaticClass()))
		{
			for (size_t i = 0; i < PC->Inventory->Inventory.ReplicatedEntries.Num(); i++)
			{
				if (PC->Inventory->Inventory.ReplicatedEntries[i].ItemDefinition->IsA(UFortWeaponMeleeItemDefinition::StaticClass()))
				{
					Pawn->EquipWeaponDefinition((UFortWeaponItemDefinition*)PC->Inventory->Inventory.ReplicatedEntries[i].ItemDefinition, PC->Inventory->Inventory.ReplicatedEntries[i].ItemGuid);
					break;
				}
			}
		}
	}

	void EquipGun()
	{
		if (!Pawn || !Pawn->CurrentWeapon)
			return;

		if (Pawn->CurrentWeapon->WeaponData->IsA(UFortWeaponMeleeItemDefinition::StaticClass()))
		{
			for (size_t i = 0; i < PC->Inventory->Inventory.ReplicatedEntries.Num(); i++)
			{
				if (!PC->Inventory->Inventory.ReplicatedEntries[i].ItemDefinition->IsA(UFortWeaponMeleeItemDefinition::StaticClass()))
				{
					Pawn->EquipWeaponDefinition((UFortWeaponItemDefinition*)PC->Inventory->Inventory.ReplicatedEntries[i].ItemDefinition, PC->Inventory->Inventory.ReplicatedEntries[i].ItemGuid);
					break;
				}
			}
		}
	}

	bool HasGun()
	{
		for (size_t i = 0; i < PC->Inventory->Inventory.ReplicatedEntries.Num(); i++)
		{
			if (!PC->Inventory->Inventory.ReplicatedEntries[i].ItemDefinition->IsA(UFortWeaponMeleeItemDefinition::StaticClass()))
				return true;
		}
		return false;
	}

	void LookDown()
	{
		if (!Pawn)
			return;

		FRotator NewRotation = Pawn->K2_GetActorRotation();
		NewRotation.Pitch -= -80.0f;
		Pawn->K2_SetActorRotation(NewRotation, true);
		PC->K2_SetFocus(Pawn);

	}

	void LookLeft()
	{
		if (!Pawn)
			return;


		FRotator NewRotation = Pawn->K2_GetActorRotation();
		NewRotation.Yaw -= 90.0f;

		Pawn->K2_SetActorRotation(NewRotation, true);
		PC->K2_SetFocus(Pawn);

	}

	void LookAtRandomDirection(bool Bloom = false)
	{
		if (!Pawn || PC->GetFocusActor())
			return;

		bool LookLeft = (UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(0.08f));

		FVector TargetDirection = LookLeft ? FVector(-1.0f, 0.0f, 0.0f) : FVector(1.0f, 0.0f, 0.0f);
	}

	virtual void Tick()
	{
		if (!TickEnabled)
			return;

		auto GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
		auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;

		// Wandering behavior variables
		static float LastWanderTime = 0.f;
		static FVector LastWanderLocation;
		static bool bIsWandering = false;

		switch (State)
		{
		case Warmup:
			Pawn->PawnStopFire(0);
			LookAtRandomDirection();
			WalkForward();
			break;
		case EBotState::MovingAroundUnbreakableObj:
			if (GameState && GameState->SafeZoneIndicator)
			{
				//Utils::Log("Bot Stuck on unbreakable obj");
				LookLeft();
				WalkForward();
			}
			break;
		case InBus:
		{
			Emoting = false;
			if (UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(0.004f))
			{
				ThankedBusDriver = true;
				PC->ThankBusDriver();
			}
			if (UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(0.004f))
			{
				JumpedFromBus = true;
				Pawn->K2_TeleportTo(((AFortGameStateAthena*)UWorld::GetWorld()->GameState)->Aircrafts[0]->K2_GetActorLocation(), {});
				Skydive();

				TargetPOI = FindNearestChest()->K2_GetActorLocation();
				State = EBotState::Landed;
			}
			break;
		}
		case SkydivingFromBus:
			GoTo(TargetPOI);
			break;
		case Landed:
			StartLooting();
			break;
		case Looting:
			if (TargetActor)
			{
				if (Pawn->IsSkydiving())
				{
					Pawn->AddMovementInput(UKismetMathLibrary::GetDefaultObj()->GetDirectionUnitVector(Pawn->K2_GetActorLocation(), TargetActor->K2_GetActorLocation()), 1, true);
					Pawn->AddMovementInput(UKismetMathLibrary::GetDefaultObj()->NegateVector(Pawn->GetActorUpVector()), 1, true);
					Running = false;
				}
				else
				{
					GoTo(TargetActor);
					Pawn->PawnStopFire(0);
					if (!FloatValue)
					{
						FloatValue = UGameplayStatics::GetDefaultObj()->GetTimeSeconds(UWorld::GetWorld());
						Pawn->bStartedInteractSearch = true;
						Pawn->OnRep_StartedInteractSearch();
					}
					else if (UGameplayStatics::GetDefaultObj()->GetTimeSeconds(UWorld::GetWorld()) - FloatValue >= 1.5f)
					{
						FloatValue = 0.f;
						//Utils::SpawnLoot((ABuildingContainer*)TargetActor);
						AFortPickup* Pickup = FindNearestPickup();

						if (Pickup && !Pickup->bPickedUp)
						{
							Pawn->ServerHandlePickup(Pickup, 0.4f, FVector(), false);
						}

						if (Pickup)
						{
							Pawn->ServerHandlePickup(Pickup, .4f, {}, true);
							if (HasGun())
							{
								/*EquipGun();*/
								State = EBotState::LookingForPlayers;
								stucktime = 0;
							}
							else
							{
								StartLooting();
							}
						}
						else
						{
							FloatValue = 0;
							StartLooting();
							//Utils::Log("Invalid Pickup");
						}
					}
				}
			}
			else
			{
				//Utils::Log("Invalid TargetActor, not enough chests????");
				StartLooting();
			}
			break;
		case MovingToZone:
			//			TargetActor = HasGun() ? FindNearestPlayer() : nullptr;
			//			if (TargetActor && Pawn->GetDistanceTo(TargetActor) < 2000)
			//				State = EBotState::LookingForPlayers;
			if (Pawn->CurrentMovementStyle != EFortMovementStyle::Sprinting) { Pawn->CurrentMovementStyle = EFortMovementStyle::Sprinting; }

			if (GameState && GameState->SafeZoneIndicator)
			{

				TargetActor = HasGun() ? FindNearestPlayer() : nullptr;

				if (PC->LineOfSightTo(TargetActor, {}, true) && Pawn->GetDistanceTo(TargetActor) < 6100)
				{
					Pawn->UnCrouch(false);
					if (UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(0.45f))
					{
						if (!HasGun())
						{
							LookAt(TargetActor, true);
							EquipPickaxe();
							GoTo(TargetActor, true);
							Pawn->PawnStartFire(0);
						}
						else
						{
							LookAt(TargetActor, true);
							EquipGun();
							Pawn->PawnStartFire(0);
						}

					}
					else
					{
						Pawn->PawnStopFire(0);
					}
				}
				else
				{
					Pawn->PawnStopFire(0);

					//Utils::Log("Bot moving to NEW Zone");
					FVector NextCenter = GameState->SafeZoneIndicator->NextCenter;
					float NextRadius = GameState->SafeZoneIndicator->NextRadius;

					GoTo(NextCenter, NextRadius);
					LookAtReal(NextCenter);
					float Distance = sqrtf(
						powf(Pawn->K2_GetActorLocation().X - NextCenter.X, 2) +
						powf(Pawn->K2_GetActorLocation().Y - NextCenter.Y, 2) +
						powf(Pawn->K2_GetActorLocation().Z - NextCenter.Z, 2)
					);

					// Check if pawn is close enough to the center
					if (Distance < 5000.f)
					{
					//	Utils::Log("Bot is CLose to zone!");
						State = EBotState::MovingAround;
					}





					if (UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(0.007f))
						Pawn->Jump();


				}

			}
			else
			{
				StartLooting();
			}

			break;



		case EBotState::MovingAround:
		{
		//	Utils::Log("MovingAround");
			using namespace std::chrono;

			static bool bStartedMovingAround = false;
			static steady_clock::time_point MovingAroundStartTime;

			if (!bStartedMovingAround)
			{
				MovingAroundStartTime = steady_clock::now();
				bStartedMovingAround = true;
			}

			auto Now = steady_clock::now();
			auto Elapsed = duration_cast<seconds>(Now - MovingAroundStartTime).count();

			if (Elapsed <= 20)
			{
				FVector NextCenter = GameState->SafeZoneIndicator->NextCenter;
				float NextRadius = GameState->SafeZoneIndicator->NextRadius;

				GoTo(NextCenter, NextRadius);
				LookAtReal(NextCenter);

				TargetActor = HasGun() ? FindNearestPlayer() : nullptr;

				if (TargetActor && Pawn->GetDistanceTo(TargetActor) < 2500)
				{

					if ((Pawn->GetDistanceTo(TargetActor) < 5000 && PC->LineOfSightTo(TargetActor, {}, true)) || (Pawn->GetDistanceTo(TargetActor) < 1500 && ((AFortPlayerPawn*)TargetActor)->Controller->LineOfSightTo(TargetActor, {}, true)))
					{
						EquipGun();
						if (UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(0.45f))
						{
							LookAt(TargetActor, true);
							Pawn->PawnStartFire(0);
						}
						else
						{
							Pawn->PawnStopFire(0);
						}
					}
				}
			}
			else
			{
				// After 45 seconds, transition to next state
				FVector NextCenter = GameState->SafeZoneIndicator->NextCenter;
				State = EBotState::MovingRandomly;
				bStartedMovingAround = false;
			}

			break;
		}


		case EBotState::MovingRandomly:
		{
		//	Utils::Log("Moving Randomly start");
			using namespace std::chrono;

			static bool bStartedRandomMovement = false;
			static steady_clock::time_point RandomMovementStartTime;

			if (!bStartedRandomMovement)
			{
				// Start the timer
				RandomMovementStartTime = steady_clock::now();
				bStartedRandomMovement = true;

				// Start forward movement
				if (Pawn->CurrentMovementStyle != EFortMovementStyle::Sprinting)
				{
					Pawn->CurrentMovementStyle = EFortMovementStyle::Sprinting;
				}

			//	Utils::Log("Walking Forward");
				WalkForward();
			}

			// Measure elapsed time
			auto Now = steady_clock::now();
			auto Elapsed = duration_cast<seconds>(Now - RandomMovementStartTime).count();

			if (Elapsed > 0 && Elapsed <= 100)
			{

//				Utils::Log("Walking Left");
				WalkLeft();
			}

			if (Elapsed > 50 && Elapsed <= 100)
			{

			//	Utils::Log("Walking Right");
				WalkRight();
			}
			else if (Elapsed > 100)
			{
				bStartedRandomMovement = false;
				State = EBotState::MovingAround;
			}

			break;
		}





		case LookingForPlayers:
		{
			if (Emoting && UGameplayStatics::GetDefaultObj()->GetTimeSeconds(UWorld::GetWorld()) - FloatValue < 3.f)
			{
				return;
			}
			else if (Emoting)
			{
				FloatValue = 0.f;
				Emoting = false;
			}

			TargetActor = HasGun() ? FindNearestPlayer() : nullptr;

			if (TargetActor && Pawn->GetDistanceTo(TargetActor) < 2500)
			{
				GoTo(TargetActor->K2_GetActorLocation());
				if (UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(0.0001f))
					Pawn->LaunchCharacter(FVector(0, 0, 10000), false, true);

				if ((Pawn->GetDistanceTo(TargetActor) < 800 && PC->LineOfSightTo(TargetActor, {}, true)) || (Pawn->GetDistanceTo(TargetActor) < 1500 && ((AFortPlayerPawn*)TargetActor)->Controller->LineOfSightTo(TargetActor, {}, true)))
				{
					EquipGun();
					if (UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(0.45f))
					{
						LookAt(TargetActor, true);
						Pawn->PawnStartFire(0);
					}
					else
					{
						Pawn->PawnStopFire(0);
					}

					if (UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(0.0001f))
						Pawn->LaunchCharacter(FVector(0, 0, 10000), false, true);
				}
				else
				{
					// Pawn->PawnStopFire(0);
				}
			}
			else
			{
				if (UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(0.0001f))
					Pawn->LaunchCharacter(FVector(0, 0, 10000), false, true);
				float TimeSpentGoingToZone = 0.0f;
				bool bGoingToZone = false;


				if (GameState->SafeZoneIndicator)
				{
					EquipPickaxe();
					LookAtReal(GameState->SafeZoneIndicator->NextCenter, GameState->SafeZoneIndicator->NextRadius);
					GoTo(GameState->SafeZoneIndicator->NextCenter, GameState->SafeZoneIndicator->NextRadius);
					State = EBotState::MovingToZone;
				}
			}



			break;
		}
		case Stuck:
		{
			if (stucktime == 0) {
				stucktime = UPTimethingy;
			}

			//Utils::Log("real " + std::to_string(Pawn->GetDistanceTo(StuckActor)));

			//		if (StuckActor->bCanBeDamaged == 0) {
	//
			//			Pawn->LaunchCharacter(FVector(0, 0, 10000), false, true);
			//			Skydive();
			//		}

			PC->StopMovement();
			Pawn->Crouch(false);
			EquipPickaxe();
			LookAt(StuckActor);
			GoTo(StuckActor);
			WalkForward();
			Run();
			Pawn->PawnStartFire(0);
			float Health = StuckActor->GetHealth();
			if (StuckActor->bDestroyed == true)
			{
				State = EBotState::LookingForPlayers;

			}
			if (UPTimethingy - stucktime >= 20)
			{
				State = EBotState::LookingForPlayers;
				stucktime = 0;
				//Utils::Log("Stuck timeout!");
				Pawn->LaunchCharacter(FVector(0, 0, 10000), false, true);
				Skydive();
			}

			if (!StuckActor || StuckActor->GetHealth() <= 1)
			{
				StuckActor = nullptr;
				State = PreviousState;
				cout << "Previous State" << (uintptr_t)PreviousState << endl;
				cout << TargetActor << endl;
				LookAt(TargetActor);
				GoTo(TargetActor);
				Pawn->UnCrouch(false);
				Pawn->PawnStopFire(0);
				State = EBotState::LookingForPlayers;
				Run();
				stucktime = 0;

			}

			break;
		}
		case MAX:
			//Utils::Log("Bot is somehow on MAX state");
			break;
		default:
			break;
		}
	}



	virtual void OnSafeZoneStateChange()
	{
		auto State = ((AFortGameStateAthena*)UWorld::GetWorld()->GameState)->GamePhaseStep;

		cout << (uintptr_t)State << endl;
	}
	virtual void OnDied(AFortPlayerStateAthena* KillerState, AActor* DamageCauser)
	{
		if (!KillerState)
			return;

	//Utils::Log("Bot Died");

		TickEnabled = false;
		FDeathInfo& DeathInfo = PlayerState->DeathInfo;

		DeathInfo.bDBNO = Pawn->bWasDBNOOnDeath;
		DeathInfo.bInitialized = true;
		DeathInfo.DeathLocation = Pawn->K2_GetActorLocation();
		DeathInfo.DeathTags = Pawn->DeathTags;
		DeathInfo.Downer = KillerState;
		DeathInfo.Distance = (KillerState->GetCurrentPawn() ? KillerState->GetCurrentPawn()->GetDistanceTo(Pawn) : ((AFortPlayerPawnAthena*)Pawn)->LastFallDistance);
		DeathInfo.FinisherOrDowner = KillerState;
		DeathInfo.DeathCause = PlayerState->ToDeathCause(DeathInfo.DeathTags, DeathInfo.bDBNO);
		PlayerState->OnRep_DeathInfo();

		if (!PC->Inventory)
			return;

		for (size_t i = 0; i < PC->Inventory->Inventory.ReplicatedEntries.Num(); i++)
		{
			if (PC->Inventory->Inventory.ReplicatedEntries[i].ItemDefinition->IsA(UFortWeaponMeleeItemDefinition::StaticClass()))
				continue;
			Utils::BotSpawnPickup(Pawn->K2_GetActorLocation(), PC->Inventory->Inventory.ReplicatedEntries[i].ItemDefinition, EFortPickupSourceTypeFlag::AI, EFortPickupSpawnSource::PlayerElimination, PC->Inventory->Inventory.ReplicatedEntries[i].Count, PC->Inventory->Inventory.ReplicatedEntries[i].LoadedAmmo);
		}

		auto KillerPC = (AFortPlayerControllerAthena*)KillerState->GetOwner();
		if (KillerPC && KillerPC->IsA(AFortPlayerControllerAthena::StaticClass()))
		{
			KillerState->KillScore++;
			//Accolades::GiveAccolade(KillerPC, GetDefFromEvent(EAccoladeEvent::Kill, KillerState->KillScore));

			for (size_t i = 0; i < KillerState->PlayerTeam->TeamMembers.Num(); i++)
			{
				((AFortPlayerStateAthena*)KillerState->PlayerTeam->TeamMembers[i]->PlayerState)->TeamKillScore++;
				((AFortPlayerStateAthena*)KillerState->PlayerTeam->TeamMembers[i]->PlayerState)->OnRep_TeamKillScore();
			}

			//if (EnableSiphon)
			//	Siphon(KillerPC);

			KillerState->ClientReportKill(PlayerState);
			KillerState->OnRep_Kills();

			auto QuestManager = KillerPC->GetQuestManager(ESubGame::Athena);
			for (size_t i = 0; i < QuestManager->CurrentQuests.Num(); i++)
			{
				if (QuestManager->HasCompletedQuest(QuestManager->CurrentQuests[i]->GetQuestDefinitionBP()))
					continue;
				auto QuestDef = QuestManager->CurrentQuests[i]->GetQuestDefinitionBP();
				for (size_t j = 0; j < QuestDef->Objectives.Num(); j++)
				{
					if (QuestManager->HasCompletedObjective(QuestDef, QuestDef->Objectives[j].ObjectiveStatHandle))
						continue;
					auto Table = QuestDef->Objectives[j].ObjectiveStatHandle.DataTable;
					//string RowName = UKismetStringLibrary::GetDefaultObj()->Conv_NameToString(QuestDef->Objectives[j].ObjectiveStatHandle.RowName).ToString();
					if (Table)
					{
						static UProperty* Prop = Utils::StaticLoadObject<UProperty>("/Script/Engine.DataTable.RowStruct");
					}
				}
			}
		}

		AFortGameModeAthena* GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
		if (GameMode->AlivePlayers.Num() + GameMode->AliveBots.Num() == 50)
		{
			for (size_t i = 0; i < GameMode->AlivePlayers.Num(); i++)
			{
				//Accolades::GiveAccolade(GameMode->AlivePlayers[i], Utils::StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_026_Survival_Default_Bronze.AccoladeId_026_Survival_Default_Bronze"));
			}
		}
		if (GameMode->AlivePlayers.Num() + GameMode->AliveBots.Num() == 25)
		{
			for (size_t i = 0; i < GameMode->AlivePlayers.Num(); i++)
			{
				//Accolades::GiveAccolade(GameMode->AlivePlayers[i], Utils::StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_027_Survival_Default_Silver.AccoladeId_027_Survival_Default_Silver"));
			}
		}
		if (GameMode->AlivePlayers.Num() + GameMode->AliveBots.Num() == 10)
		{
			for (size_t i = 0; i < GameMode->AlivePlayers.Num(); i++)
			{
				//Accolades::GiveAccolade(GameMode->AlivePlayers[i], Utils::StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_028_Survival_Default_Gold.AccoladeId_028_Survival_Default_Gold"));
			}
		}

		auto GameState = ((AFortGameStateAthena*)UWorld::GetWorld()->GameState);
		auto KillerPawn = KillerPC->MyFortPawn;

		if (GameState->PlayersLeft && GameState->PlayerBotsLeft <= 1 && !GameState->IsRespawningAllowed(PlayerState))
		{
			if (KillerPawn != Pawn)
			{
				UFortWeaponItemDefinition* KillerWeaponDef = nullptr;

				if (auto ProjectileBase = Utils::Cast<AFortProjectileBase>(DamageCauser))
					KillerWeaponDef = ((AFortWeapon*)ProjectileBase->GetOwner())->WeaponData;
				if (auto Weapon = Utils::Cast<AFortWeapon>(DamageCauser))
					KillerWeaponDef = Weapon->WeaponData;

				KillerPC->PlayWinEffects(KillerPawn, KillerWeaponDef, DeathInfo.DeathCause, false);
				KillerPC->ClientNotifyWon(KillerPawn, KillerWeaponDef, DeathInfo.DeathCause);
				KillerPC->ClientNotifyTeamWon(KillerPawn, KillerPawn->CurrentWeapon ? KillerPawn->CurrentWeapon->WeaponData : nullptr, DeathInfo.DeathCause);
			}

			KillerState->Place = 1;
			KillerState->OnRep_Place();
			GameState->WinningPlayerState = KillerState;
			GameState->WinningTeam = KillerState->TeamIndex;
			GameState->OnRep_WinningPlayerState();
			GameState->OnRep_WinningTeam();
			GameMode->EndMatch();
		}
	}
};




inline vector<Bot*> SpawnedBots{};