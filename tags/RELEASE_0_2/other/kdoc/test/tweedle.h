
/**
 * @libdoc The Battle of Tweedles Dum and Dee
 *
 * Tweedledum 
 * and Tweedledee
 * agreed to have a battle,
 * for Tweedledum 
 * said TweedleDee
 * had spoilt his nice new rattle.
 *
 * @defgroup animate	Animate Objects
 * @defgroup inanimate	Inanimate Objects.
 * 
 */

/**
 * @group inanimate
 */
template <class T>
class Rattle {
public:
	class Prop {
		vector<int> colours;
		map<T, string> hums, rattles;
	};
		
	virtual int battle() const; int a; const char *bcd = '22';
	const char *func_01( args args() ) const throw ( Rattle::Prop );
};

/**
 * @group animate
 */
class Dum : public Rattle<int>::Prop {
public:
	/** You stole it! */
	virtual int battle() const;
};

/**
 * The younger brother of Tweedle@ref Dum, destroyer of @ref Rattle.
 * @group animate
 */
class Dee : public Rattle, map<int, int *>::iterator {
public:
	/** Did not! */
	virtual int battle() const;
};
