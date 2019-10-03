/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2001-2002 by NaN Holding BV.
 * All rights reserved.
 *
 * The Original Code is: all of this file.
 *
 * Contributor(s): none yet.
 *
 * ***** END GPL LICENSE BLOCK *****
 */

/** \file KX_GameObject.h
 *  \ingroup ketsji
 *  \brief General KX game object.
 */

#ifndef __KX_GAMEOBJECT_H__
#define __KX_GAMEOBJECT_H__

#ifdef _MSC_VER
   /* get rid of this stupid "warning 'this' used in initialiser list", generated by VC when including Solid/Sumo */
#  pragma warning (disable:4355)
#endif 

#include <stddef.h>

#include "EXP_ListValue.h"
#include "SCA_IObject.h"
#include "SG_Node.h"
#include "MT_Transform.h"
#include "KX_Scene.h"
#include "KX_KetsjiEngine.h" /* for m_anim_framerate */
#include "DNA_constraint_types.h" /* for constraint replication */
#include "DNA_object_types.h"
#include "SCA_LogicManager.h" /* for ConvertPythonToGameObject to search object names */
#include "BLI_math.h"

//Forward declarations.
struct KX_ClientObjectInfo;
class KX_RayCast;
class RAS_MeshObject;
class PHY_IPhysicsEnvironment;
class PHY_IPhysicsController;
class BL_ActionManager;
struct Object;
class KX_ObstacleSimulation;
class KX_CollisionContactPointList;
struct bAction;


struct Mesh;

#ifdef WITH_PYTHON
/* utility conversion function */
bool ConvertPythonToGameObject(SCA_LogicManager *logicmgr, PyObject *value, KX_GameObject **object, bool py_none_ok, const char *error_prefix);
#endif

#ifdef USE_MATHUTILS
void KX_GameObject_Mathutils_Callback_Init(void);
#endif

/**
 * KX_GameObject is the main class for dynamic objects.
 */
class KX_GameObject : public SCA_IObject
{
	Py_Header
protected:

	/* EEVEE INTEGRATION */
	float m_savedObmat[4][4];
	float m_prevObmat[4][4];
	bool m_castShadows;
	bool m_isReplica;
	bool m_staticObject;
  bool m_useCopy;
  bool m_visibleAtGameStart;
	/* END OF EEVEE INTEGRATION */



	KX_ClientObjectInfo*				m_pClient_info;
	std::string							m_name;
	std::string							m_text;
	int									m_layer;
	std::vector<RAS_MeshObject*>		m_meshes;
	struct Object*						m_pBlenderObject;
	struct Object*						m_pBlenderGroupObject;
	
	bool								m_bIsNegativeScaling;
	MT_Vector4							m_objectColor;

	// Bit fields for user control over physics collisions
	unsigned short						m_userCollisionGroup;
	unsigned short						m_userCollisionMask;

	// visible = user setting
	// culled = while rendering, depending on camera
	bool       							m_bVisible; 
	bool								m_bOccluder;

	PHY_IPhysicsController*				m_pPhysicsController;
	SG_Node*							m_pSGNode;

	std::vector<bRigidBodyJointConstraint*>	m_constraints;

	CListValue<KX_GameObject> *m_pInstanceObjects;
	KX_GameObject*						m_pDupliGroupObject;

	// The action manager is used to play/stop/update actions
	BL_ActionManager*					m_actionManager;

	BL_ActionManager* GetActionManager();

public:

	/* EEVEE INTEGRATION */

	void TagForUpdate();
	void ReplicateBlenderObject();
	void HideOriginalObject();
	void RemoveReplicaObject();
	bool IsStatic();
	void RecalcGeometry();
  void UseCopy();
	/* END OF EEVEE INTEGRATION */


	/**
	 * KX_GameObject custom infos for ray cast, it contains property name,
	 * collision mask, xray flag and hited object.
	 * This structure is created during ray cast and passed as argument 
	 * "data" to functions KX_GameObject::NeedRayCast and KX_GameObject::RayHit.
	 */
	struct RayCastData;

	/**
	 * Helper function for modules that can't include KX_ClientObjectInfo.h
	 */
	static KX_GameObject* GetClientObject(KX_ClientObjectInfo* info);

#ifdef WITH_PYTHON
	// Python attributes that wont convert into CValue
	//
	// there are 2 places attributes can be stored, in the CValue,
	// where attributes are converted into BGE's CValue types
	// these can be used with property actuators
	//
	// For the python API, For types that cannot be converted into CValues (lists, dicts, GameObjects)
	// these will be put into "m_attr_dict", logic bricks cannot access them.
	//
	// rules for setting attributes.
	//
	// * there should NEVER be a CValue and a m_attr_dict attribute with matching names. get/sets make sure of this.
	// * if CValue conversion fails, use a PyObject in "m_attr_dict"
	// * when assigning a value, first see if it can be a CValue, if it can remove the "m_attr_dict" and set the CValue
	//
	PyObject*							m_attr_dict;
	PyObject*							m_collisionCallbacks;
#endif

	virtual void	/* This function should be virtual - derived classed override it */
	Relink(
		std::map<SCA_IObject *, SCA_IObject *>& map
	);

	/**
	 * Update the blender object obmat field from the object world position
	 * if blendobj is nullptr, update the object pointed by m_pBlenderObject
	 * The user must take action to restore the matrix before leaving the GE.
	 * Used in Armature evaluation
	 */
		void
	UpdateBlenderObjectMatrix(Object* blendobj=nullptr);

	/**
	 * Used for constraint replication for group instances.
	 * The list of constraints is filled during data conversion.
	 */
	void AddConstraint(bRigidBodyJointConstraint *cons);
	std::vector<bRigidBodyJointConstraint*> GetConstraints();
	void ClearConstraints();

	/** 
	 * Get a pointer to the game object that is the parent of 
	 * this object. Or nullptr if there is no parent. The returned
	 * object is part of a reference counting scheme. Calling
	 * this function ups the reference count on the returned 
	 * object. It is the responsibility of the caller to decrement
	 * the reference count when you have finished with it.
	 */
		KX_GameObject*
	GetParent(
	);

	/** 
	 * Sets the parent of this object to a game object
	 */
	void SetParent(KX_GameObject *obj, bool addToCompound=true, bool ghost=true);

	/** 
	 * Removes the parent of this object to a game object
	 */
	void RemoveParent();

	/*********************************
	 * group reference API
	 *********************************/

		KX_GameObject*
	GetDupliGroupObject(
	);

		CListValue<KX_GameObject>*
	GetInstanceObjects(
	);

		void
	SetDupliGroupObject(KX_GameObject*
	);

		void
	AddInstanceObjects(KX_GameObject*
	);
		
		void 
	RemoveDupliGroupObject(
	);

		void
	RemoveInstanceObject(KX_GameObject*
	);
	/*********************************
	 * Animation API
	 *********************************/

	/**
	 * Adds an action to the object's action manager
	 */
	bool PlayAction(const std::string& name,
					float start,
					float end,
					short layer=0,
					short priority=0,
					float blendin=0.f,
					short play_mode=0,
					float layer_weight=0.f,
					short ipo_flags=0,
					float playback_speed=1.f,
					short blend_mode=0);

	/**
	 * Gets the current frame of an action
	 */
	float GetActionFrame(short layer);

	/**
	 * Gets the name of the current action
	 */
	const std::string GetActionName(short layer);

	/**
	 * Sets the current frame of an action
	 */
	void SetActionFrame(short layer, float frame);

	/**
	 * Gets the currently running action on the given layer
	 */
	bAction *GetCurrentAction(short layer);

	/**
	 * Sets play mode of the action on the given layer
	 */
	void SetPlayMode(short layer, short mode);

	/**
	 * Stop playing the action on the given layer
	 */
	void StopAction(short layer);

	/**
	 * Remove playing tagged actions.
	 */
	void RemoveTaggedActions();

	/**
	 * Check if an action has finished playing
	 */
	bool IsActionDone(short layer);

	/**
	 * Kick the object's action manager
	 * \param curtime The current time used to compute the actions frame.
	 * \param applyObject Set to true if the actions must transform this object, else it only manages actions' frames.
	 */
	void UpdateActionManager(float curtime, bool applyObject);

	/*********************************
	 * End Animation API
	 *********************************/

	/**
	 * Construct a game object. This class also inherits the 
	 * default constructors - use those with care!
	 */

	KX_GameObject(
		void* sgReplicationInfo,
		SG_Callbacks callbacks
	);

	virtual 
	~KX_GameObject(
	);

	/** 
	 * \section Stuff which is here due to poor design.
	 * Inherited from CValue and needs an implementation. 
	 * Do not expect these functions do to anything sensible.
	 */

	/**
	 * Inherited from CValue
	 */
	virtual const std::string GetText();

	/**
	 * \section Inherited from CValue. These are the useful
	 * part of the CValue interface that this class implements. 
	 */

	/**
	 * Inherited from CValue -- returns the name of this object.
	 */
	virtual std::string GetName();

	/**
	 * Inherited from CValue -- set the name of this object.
	 */
	virtual void SetName(const std::string& name);

	/** 
	 * Inherited from CValue -- return a new copy of this
	 * instance allocated on the heap. Ownership of the new 
	 * object belongs with the caller.
	 */
	virtual CValue *GetReplica();
	
	/**
	 * Makes sure any internal 
	 * data owned by this class is deep copied. Called internally
	 */
	virtual void ProcessReplica();

	/** 
	 * Return the linear velocity of the game object.
	 */
		MT_Vector3 
	GetLinearVelocity(
		bool local=false
	);

	/** 
	 * Return the linear velocity of a given point in world coordinate
	 * but relative to center of object ([0,0,0]=center of object)
	 */
		MT_Vector3 
	GetVelocity(
		const MT_Vector3& position
	);

	/**
	 * Return the mass of the object
	 */
		MT_Scalar
	GetMass();

	/**
	 * Return the local inertia vector of the object
	 */
		MT_Vector3
	GetLocalInertia();

	/** 
	 * Return the angular velocity of the game object.
	 */
		MT_Vector3 
	GetAngularVelocity(
		bool local=false
	);

	/** 
	 * Align the object to a given normal.
	 */
		void 
	AlignAxisToVect(
		const MT_Vector3& vect,
		int axis = 2,
		float fac = 1.0
	);

	/** 
	 * Quick'n'dirty obcolor ipo stuff
	 */

		void
	SetObjectColor(
		const MT_Vector4& rgbavec
	);

		const MT_Vector4&
	GetObjectColor();

		void 
	ResolveCombinedVelocities(
		const MT_Vector3 & lin_vel,
		const MT_Vector3 & ang_vel,
		bool lin_vel_local,
		bool ang_vel_local
	);

	/**
	 * \return a pointer to the physics controller owned by this class.
	 */

	PHY_IPhysicsController* GetPhysicsController();

	void SetPhysicsController(PHY_IPhysicsController *physicscontroller)
	{ 
		m_pPhysicsController = physicscontroller;
	}
	/// Return true when the game object is a .
	virtual bool IsDeformable() const
	{
		return false;
	}

	/** Set the object's collison group
	 * \param filter The group bitfield
	 */
	void SetUserCollisionGroup(unsigned short filter);

	/** Set the object's collison mask
	 * \param filter The mask bitfield
	 */
	void SetUserCollisionMask(unsigned short mask);
	unsigned short GetUserCollisionGroup();
	unsigned short GetUserCollisionMask();
	/**
	 * Extra broadphase check for user controllable collisions
	 */
	bool CheckCollision(KX_GameObject *other);

	/**
	 * \section Coordinate system manipulation functions
	 */

	void	NodeSetLocalPosition(const MT_Vector3& trans	);

	void	NodeSetLocalOrientation(const MT_Matrix3x3& rot	);
	void	NodeSetGlobalOrientation(const MT_Matrix3x3& rot	);

	void	NodeSetLocalScale(	const MT_Vector3& scale	);
	void	NodeSetWorldScale(	const MT_Vector3& scale );

	void	NodeSetRelativeScale(	const MT_Vector3& scale	);

	// adapt local position so that world position is set to desired position
	void	NodeSetWorldPosition(const MT_Vector3& trans);

		void
	NodeUpdateGS(
		double time
	);

	const MT_Matrix3x3& NodeGetWorldOrientation(  ) const;
	const MT_Vector3& NodeGetWorldScaling(  ) const;
	const MT_Vector3& NodeGetWorldPosition(  ) const;
	MT_Transform NodeGetWorldTransform() const;

	const MT_Matrix3x3& NodeGetLocalOrientation(  ) const;
	const MT_Vector3& NodeGetLocalScaling(  ) const;
	const MT_Vector3& NodeGetLocalPosition(  ) const;
	MT_Transform NodeGetLocalTransform() const;

	/**
	 * \section scene graph node accessor functions.
	 */

	SG_Node*	GetSGNode(	) 
	{ 
		return m_pSGNode;
	}

	const 	SG_Node* GetSGNode(	) const
	{ 
		return m_pSGNode;
	}

	/**
	 * \section blender object accessor functions.
	 */

	struct Object* GetBlenderObject( )
	{
		return m_pBlenderObject;
	}

	void SetBlenderObject(struct Object *obj);

	struct Object* GetBlenderGroupObject( )
	{
		return m_pBlenderGroupObject;
	}

	void SetBlenderGroupObject(struct Object* obj)
	{
		m_pBlenderGroupObject = obj;
	}
	
	bool IsDupliGroup()
	{ 
		return (m_pBlenderObject &&
				(m_pBlenderObject->transflag & OB_DUPLICOLLECTION) &&
				m_pBlenderObject->instance_collection != nullptr) ? true : false;
	}

	/**
	 * Set the Scene graph node for this game object.
	 * warning - it is your responsibility to make sure
	 * all controllers look at this new node. You must
	 * also take care of the memory associated with the
	 * old node. This class takes ownership of the new
	 * node.
	 */
		void	SetSGNode(SG_Node* node	)
		{ 
			m_pSGNode = node; 
		}
	
	/// Is it a dynamic/physics object ?
	bool IsDynamic() const;

	bool IsDynamicsSuspended() const;

	/**
	 * Check if this object has a vertex parent relationship
	 */
	bool IsVertexParent( )
	{
		return (m_pSGNode && m_pSGNode->GetSGParent() && m_pSGNode->GetSGParent()->IsVertexParent());
	}

	/// \see KX_RayCast
	bool RayHit(KX_ClientObjectInfo *client, KX_RayCast *result, RayCastData *rayData);
	/// \see KX_RayCast
	bool NeedRayCast(KX_ClientObjectInfo *client, RayCastData *rayData);


	/**
	 * \section Physics accessors for this node.
	 *
	 * All these calls get passed directly to the physics controller 
	 * owned by this object.
	 * This is real interface bloat. Why not just use the physics controller
	 * directly? I think this is because the python interface is in the wrong
	 * place.
	 */

		void
	ApplyForce(
		const MT_Vector3& force,	bool local
	);

		void
	ApplyTorque(
		const MT_Vector3& torque,
		bool local
	);

		void
	ApplyRotation(
		const MT_Vector3& drot,
		bool local
	);

		void
	ApplyMovement(
		const MT_Vector3& dloc,
		bool local
	);

		void
	addLinearVelocity(
		const MT_Vector3& lin_vel,
		bool local
	);

		void
	setLinearVelocity(
		const MT_Vector3& lin_vel,
		bool local
	);

		void
	setAngularVelocity(
		const MT_Vector3& ang_vel,
		bool local
	);

	virtual float	getLinearDamping() const;
	virtual float	getAngularDamping() const;
	virtual void	setLinearDamping(float damping);
	virtual void	setAngularDamping(float damping);
	virtual void	setDamping(float linear, float angular);

	/**
	 * Update the physics object transform based upon the current SG_Node
	 * position.
	 */
		void
	UpdateTransform(
	);

	static void UpdateTransformFunc(SG_Node* node, void* gameobj, void* scene);

	/**
	 * only used for sensor objects
	 */
	void SynchronizeTransform();

	static void SynchronizeTransformFunc(SG_Node* node, void* gameobj, void* scene);

	/**
	 * Function to set IPO option at start of IPO
	 */ 
		void
	InitIPO(
		bool ipo_as_force,
		bool ipo_add,
		bool ipo_local
	);

	/**
	 * Odd function to update an ipo. ???
	 */ 
		void
	UpdateIPO(
		float curframetime,
		bool recurse
	);

	/**
	 * \section Mesh accessor functions.
	 */
	
	/**
	 * Update buckets with data about the mesh after
	 * creating or duplicating the object, changing
	 * visibility, object color, .. .
	 */
	virtual void UpdateBuckets();

	/**
	 * Clear the meshes associated with this class
	 * and remove from the bucketing system.
	 * Don't think this actually deletes any of the meshes.
	 */
		void
	RemoveMeshes(
	);

	/**
	 * Add a mesh to the set of meshes associated with this
	 * node. Meshes added in this way are not deleted by this class.
	 * Make sure you call RemoveMeshes() before deleting the
	 * mesh though,
	 */
		void
	AddMesh(
		RAS_MeshObject* mesh
	) {
		m_meshes.push_back(mesh);
	}

	/**
	 * Pick out a mesh associated with the integer 'num'.
	 */
		RAS_MeshObject*
	GetMesh(
		int num
	) const { 
		return m_meshes[num]; 
	}

	/**
	 * Return the number of meshes currently associated with this
	 * game object.
	 */
		int
	GetMeshCount(
	) const { 
		return m_meshes.size(); 
	}

	/// Return true when the object can be culled.
	bool UseCulling() const;

	/**
	 * Was this object marked visible? (only for the explicit
	 * visibility system).
	 */
		bool
	GetVisible(
		void
	);

	/**
	 * Set visibility flag of this object
	 */
		void
	SetVisible(
		bool b,
		bool recursive
	);
	
	/**
	 * Is this object an occluder?
	 */
	inline bool
	GetOccluder(
		void
	) { return m_bOccluder; }

	/**
	 * Set occluder flag of this object
	 */
	void
	SetOccluder(
		bool v,
		bool recursive
	);
	
	/**
	 * Change the layer of the object (when it is added in another layer
	 * than the original layer)
	 */
	virtual void
	SetLayer(
		int l
	);

	/**
	 * Get the object layer
	 */
		int
	GetLayer(
		void
	);

	/**
	 * Get the negative scaling state
	 */
		bool
	IsNegativeScaling(
		void
	) { return m_bIsNegativeScaling; }

	/**
	 * \section Logic bubbling methods.
	 */

	void RegisterCollisionCallbacks();
	void UnregisterCollisionCallbacks();
	void RunCollisionCallbacks(KX_GameObject *collider, KX_CollisionContactPointList& contactPointList);
	/**
	 * Stop making progress
	 */
	void Suspend(void);

	/**
	 * Resume making progress
	 */
	void Resume(void);

	/**
	 * add debug object to the debuglist.
	 */
	void SetUseDebugProperties(bool debug, bool recursive);

	KX_ClientObjectInfo* getClientInfo() { return m_pClient_info; }
	
	CListValue<KX_GameObject> *GetChildren();
	CListValue<KX_GameObject> *GetChildrenRecursive();

	KX_Scene*	GetScene();

#ifdef WITH_PYTHON
	/**
	 * \section Python interface functions.
	 */
	virtual PyObject *py_repr(void)
	{
		return PyUnicode_FromStdString(GetName());
	}

	KX_PYMETHOD_O(KX_GameObject,SetWorldPosition);
	KX_PYMETHOD_VARARGS(KX_GameObject, ApplyForce);
	KX_PYMETHOD_VARARGS(KX_GameObject, ApplyTorque);
	KX_PYMETHOD_VARARGS(KX_GameObject, ApplyRotation);
	KX_PYMETHOD_VARARGS(KX_GameObject, ApplyMovement);
	KX_PYMETHOD_VARARGS(KX_GameObject,GetLinearVelocity);
	KX_PYMETHOD_VARARGS(KX_GameObject,SetLinearVelocity);
	KX_PYMETHOD_VARARGS(KX_GameObject,GetAngularVelocity);
	KX_PYMETHOD_VARARGS(KX_GameObject,SetAngularVelocity);
	KX_PYMETHOD_VARARGS(KX_GameObject,GetVelocity);
	KX_PYMETHOD_VARARGS(KX_GameObject,SetDamping);

	KX_PYMETHOD_NOARGS(KX_GameObject,GetReactionForce);


	KX_PYMETHOD_NOARGS(KX_GameObject,GetVisible);
	KX_PYMETHOD_VARARGS(KX_GameObject,SetVisible);
	KX_PYMETHOD_VARARGS(KX_GameObject,SetOcclusion);
	KX_PYMETHOD_NOARGS(KX_GameObject,GetState);
	KX_PYMETHOD_O(KX_GameObject,SetState);
	KX_PYMETHOD_VARARGS(KX_GameObject,AlignAxisToVect);
	KX_PYMETHOD_O(KX_GameObject,GetAxisVect);
	KX_PYMETHOD_VARARGS(KX_GameObject,SuspendPhysics);
	KX_PYMETHOD_NOARGS(KX_GameObject,RestorePhysics);
	KX_PYMETHOD_VARARGS(KX_GameObject,SuspendDynamics);
	KX_PYMETHOD_NOARGS(KX_GameObject,RestoreDynamics);
	KX_PYMETHOD_NOARGS(KX_GameObject,EnableRigidBody);
	KX_PYMETHOD_NOARGS(KX_GameObject,DisableRigidBody);
	KX_PYMETHOD_VARARGS(KX_GameObject,ApplyImpulse);
	KX_PYMETHOD_O(KX_GameObject,SetCollisionMargin);
	KX_PYMETHOD_NOARGS(KX_GameObject,GetParent);
	KX_PYMETHOD_VARARGS(KX_GameObject,SetParent);
	KX_PYMETHOD_NOARGS(KX_GameObject,RemoveParent);
	KX_PYMETHOD_NOARGS(KX_GameObject,GetChildren);
	KX_PYMETHOD_NOARGS(KX_GameObject,GetChildrenRecursive);
	KX_PYMETHOD_VARARGS(KX_GameObject,GetMesh);
	KX_PYMETHOD_NOARGS(KX_GameObject,GetPhysicsId);
	KX_PYMETHOD_NOARGS(KX_GameObject,GetPropertyNames);
	KX_PYMETHOD_NOARGS(KX_GameObject,EndObject);
	KX_PYMETHOD_DOC(KX_GameObject,rayCastTo);
	KX_PYMETHOD_DOC(KX_GameObject,rayCast);
	KX_PYMETHOD_DOC_O(KX_GameObject,getDistanceTo);
	KX_PYMETHOD_DOC_O(KX_GameObject,getVectTo);
	KX_PYMETHOD_DOC_VARARGS(KX_GameObject, sendMessage);
	KX_PYMETHOD_VARARGS(KX_GameObject, ReinstancePhysicsMesh);
	KX_PYMETHOD_O(KX_GameObject, ReplacePhysicsShape);
  KX_PYMETHOD_VARARGS(KX_GameObject, UpdatePhysicsShape);
	KX_PYMETHOD_DOC(KX_GameObject, addDebugProperty);

	KX_PYMETHOD_DOC(KX_GameObject, playAction);
	KX_PYMETHOD_DOC(KX_GameObject, stopAction);
	KX_PYMETHOD_DOC(KX_GameObject, getActionFrame);
	KX_PYMETHOD_DOC(KX_GameObject, getActionName);
	KX_PYMETHOD_DOC(KX_GameObject, setActionFrame);
	KX_PYMETHOD_DOC(KX_GameObject, isPlayingAction);


	KX_PYMETHOD_DOC(KX_GameObject, recalcGeometry);
  KX_PYMETHOD_DOC(KX_GameObject, recalcTransform);
	
	/* Dict access */
	KX_PYMETHOD_VARARGS(KX_GameObject,get);
	
	/* attributes */
	static PyObject*	pyattr_get_name(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_name(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_parent(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef);

	static PyObject*	pyattr_get_group_object(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef);
	static PyObject*	pyattr_get_group_members(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef);
	static PyObject*	pyattr_get_scene(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef);

	static PyObject*	pyattr_get_life(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef);
	static PyObject*	pyattr_get_mass(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_mass(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_is_suspend_dynamics(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef);
	static PyObject*	pyattr_get_lin_vel_min(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_lin_vel_min(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_lin_vel_max(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_lin_vel_max(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_ang_vel_min(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_ang_vel_min(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_ang_vel_max(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_ang_vel_max(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_layer(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_layer(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_visible(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_visible(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_worldPosition(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_worldPosition(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_localPosition(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_localPosition(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_localInertia(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_localInertia(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_worldOrientation(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_worldOrientation(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_localOrientation(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_localOrientation(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_worldScaling(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_worldScaling(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_localScaling(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_localScaling(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_localTransform(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_localTransform(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_worldTransform(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_worldTransform(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_worldLinearVelocity(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_worldLinearVelocity(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_localLinearVelocity(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_localLinearVelocity(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_worldAngularVelocity(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_worldAngularVelocity(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_localAngularVelocity(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_localAngularVelocity(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_timeOffset(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_timeOffset(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_state(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_state(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_meshes(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef);
	static PyObject*	pyattr_get_children(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef);
	static PyObject*	pyattr_get_children_recursive(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef);
	static PyObject*	pyattr_get_attrDict(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef);
	static PyObject*	pyattr_get_obcolor(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_obcolor(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_collisionCallbacks(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_collisionCallbacks(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_collisionGroup(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_collisionGroup(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_collisionMask(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_collisionMask(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_debug(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_debug(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_debugRecursive(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_debugRecursive(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_linearDamping(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_linearDamping(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef, PyObject *value);
	static PyObject*	pyattr_get_angularDamping(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef);
	static int			pyattr_set_angularDamping(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef, PyObject *value);

	/* Experimental! */
	static PyObject*	pyattr_get_sensors(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef);
	static PyObject*	pyattr_get_controllers(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef);
	static PyObject*	pyattr_get_actuators(PyObjectPlus *self_v, const KX_PYATTRIBUTE_DEF *attrdef);
	
	/* getitem/setitem */
	static PyMappingMethods	Mapping;
	static PySequenceMethods	Sequence;
#endif
};



#endif  /* __KX_GAMEOBJECT_H__ */
