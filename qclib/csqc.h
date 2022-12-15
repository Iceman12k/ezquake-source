


typedef struct {
	float modelindex;
	vec3_t absmin;
	vec3_t absmax;
	float ltime;
	float lastruntime;
	float movetype;
	float solid;
	vec3_t origin;
	vec3_t oldorigin;
	vec3_t velocity;
	vec3_t angles;
	vec3_t avelocity;
	string_t classname;
	string_t model;
	float frame;
	float skin;
	float effects;
	vec3_t mins;
	vec3_t maxs;
	vec3_t size;
	func_t touch;
	func_t use;
	func_t think;
	func_t blocked;
	float nextthink;
	int groundentity;
	float health;
	float frags;
	float weapon;
	string_t weaponmodel;
	float weaponframe;
	float currentammo;
	float ammo_shells;
	float ammo_nails;
	float ammo_rockets;
	float ammo_cells;
	float items;
	float takedamage;
	int chain;
	float deadflag;
	vec3_t view_ofs;
	float button0;
	float button1;
	float button2;
	float impulse;
	float fixangle;
	vec3_t v_angle;
	string_t netname;
	int enemy;
	float flags;
	float colormap;
	float team;
	float max_health;
	float teleport_time;
	float armortype;
	float armorvalue;
	float waterlevel;
	float watertype;
	float ideal_yaw;
	float yaw_speed;
	int aiment;
	int goalentity;
	float spawnflags;
	string_t target;
	string_t targetname;
	float dmg_take;
	float dmg_save;
	int dmg_inflictor;
	int owner;
	vec3_t movedir;
	string_t message;
	float sounds;
	string_t noise;
	string_t noise1;
	string_t noise2;
	string_t noise3;
} csqcentvars_t;



typedef struct {
	qbool		free;
	//link_t		area;			// linked to a division node or leaf

	int         entnum;

	csqcentvars_t	*v;

	int			num_leafs;
	//short		leafnums[MAX_ENT_LEAFS];

	entity_state_t	baseline;

	float		freetime;		// sv.time when the object was freed
	double		lastruntime;	// sv.time when SV_RunEntity was last called for this edict (Tonik)
} csqcedict_t;



typedef struct {

	int			num_edicts;
	csqcedict_t	edicts[MAX_EDICTS];

	double		time;

} csqcworld_t;
















