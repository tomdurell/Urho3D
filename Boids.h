#pragma once
#include <Urho3D/Engine/Application.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Core/ProcessUtils.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Graphics/AnimatedModel.h>
#include <Urho3D/Graphics/AnimationController.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Light.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/Input/Controls.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Container/Str.h>






namespace Urho3D
{
	class Node;
	class Scene;
	class RigidBody;
	class CollisionShape;
	class ResourceCache;
}
// All Urho3D classes reside in namespace Urho3D
using namespace Urho3D;

//how many boids do we want in a set
static const int NumBoids = 80;

class Boid
{
	Node* pNode;
	RigidBody* pRigidBody;
	CollisionShape* pCollisionShape;
	StaticModel* pObject;

	//force initalisation
	float Range_AttractForce = 200.0f;
	float Range_RepelForce = 20.0f;
	float Range_AlignForce = 5.0f;
	float AttractFroce_Vmax = 5.0f;
	float AttractForce_Factor = 10.0f;
	float RepelForce_Factor = 2.0f;
	float AlignForce_Factor = 2.0f;

	public:
		Vector3 Force;

		//constructor
		Boid() 
		{
			pNode = NULL;
			pRigidBody = NULL;
			pCollisionShape = NULL;
			pObject = NULL;
		}

		//Destructor
		~Boid()
		{
		}

		void Initialise(ResourceCache* pRes, Scene* pScene)
		{
			pNode = pScene->CreateChild("boidNode");
			pNode->SetPosition(Vector3(Random(180.0f) - 90.0f, 0.0f, Random(180.0f) - 90.0f));
			pNode->SetRotation(Quaternion(0.0f, Random(360.0f), 0.0f));
			pNode->SetScale(2.0f + Random(5.0f));
			pObject = pNode->CreateComponent<StaticModel>();
			pObject->SetModel(pRes ->GetResource<Model>("Models/Cone.mdl")); // model for the boid
			pObject->SetMaterial(pRes->GetResource<Material>("Materials/GreenTransparent.xml")); // material "skin" for the boid
			pObject->SetCastShadows(true);
			pRigidBody = pNode->CreateComponent<RigidBody>();
			pRigidBody->SetCollisionLayer(2);
			pCollisionShape = pNode->CreateComponent<CollisionShape>();
			pCollisionShape->SetTriangleMesh(pObject->GetModel(), 0);
			pRigidBody->SetUseGravity(false);
			pRigidBody->SetPosition(Vector3(Random(180.0f) - 90.0f, 30.0f,Random(180.0f) - 90.0f)); // why do we use this if transformations should be performed on the node
			pRigidBody->SetMass(10.0f);
			//pRigidBody->SetLinearVelocity(Vector3(Random(20) - 20.0f, 0.0f, Random(20.0f) - 20.0f)); // possible error not sure if this is the right place to be setting the velocity, could be on the node? , also need to learn random function
			
		}

		void ComputeForce(Boid* pBoid, Scene* pScene)
		{
			Vector3 CoM; //centre of mass, accumulated total
			Vector3 CoMrepel;
			int attractNeighboursCount = 0 , repelNeighboursCount = 0; //count number of neigbours
			//set the force member variable to zero 
			Force = Vector3(0, 0, 0);
			//Search Neighbourhood
			for (int i = 0; i < NumBoids; i++)
			{
				//the current boid?
				if (this == &pBoid[i]) continue;
				//sep = vector position of this boid from current boid
				Vector3 sep = pRigidBody->GetPosition() - pBoid[i].pRigidBody->GetPosition(); //compares this boid to all boids one at a time, find the middle point of the two
				float d = sep.Length(); //distance of boid

				if (d < Range_AttractForce)
					{
						//with range, so is a neighbour, add its position to the centre of mass for the group 
						CoM += pBoid[i].pRigidBody->GetPosition();
						attractNeighboursCount++;
					}
				
				if (d < Range_RepelForce)
				{
					//within range, so is a nieghbour that is TOO close
					CoMrepel += pBoid[i].pRigidBody->GetPosition();
					repelNeighboursCount++;

				}
			}

			//attractive force component
			if (attractNeighboursCount > 0)
			{
				//Urho3D::String debug = Urho3D::String(Force.x_);
				//Log::WriteRaw("GotHere");
				CoM /= attractNeighboursCount;
				Vector3 dir = (CoM - pRigidBody->GetPosition()).Normalized();
				Vector3 vDesired = dir*AttractForce_Factor;
				Force += (vDesired - pRigidBody->GetLinearVelocity())*AttractForce_Factor;
			}			if (repelNeighboursCount > 0)
			{
				Vector3 dir = (CoMrepel + pRigidBody->GetPosition()).Normalized();
				Vector3 vDesired = dir*RepelForce_Factor;
				Force += (vDesired - pRigidBody->GetLinearVelocity())*RepelForce_Factor;
			}
		}

		void Update(float frameUpdateTime)
		{
			pRigidBody->ApplyForce(Force);
			Vector3 vel = pRigidBody->GetLinearVelocity();
			float d = vel.Length();
			if (d < 10.0f)
			{
				d = 10.0f;
				pRigidBody->SetLinearVelocity(vel.Normalized()*d);
			}
			else if (d > 50.0f)
			{
				d = 50.0f;
				pRigidBody->SetLinearVelocity(vel.Normalized()*d);
			}
			Vector3 vn = vel.Normalized();
			Vector3 cp = -vn.CrossProduct(Vector3(0.0f, 1.0f, 0.0f));
			float dp = cp.DotProduct(vn);
			pRigidBody->SetRotation(Quaternion(Acos(dp), cp));
			Vector3 p = pRigidBody->GetPosition();
			if (p.y_ < 10.0f)
			{
				p.y_ = 10.0f;
				pRigidBody->SetPosition(p);
			}
			else if (p.y_ > 50.0f)
			{
				p.y_ = 50.0f;
				pRigidBody->SetPosition(p);
				
			}
		}
		



};

class BoidSet
{
	

public:
	Boid boidList[NumBoids];
	BoidSet() {};

	void Initialise(ResourceCache *pRes, Scene *pScene)
	{
		for (int counter = 0; counter <= NumBoids; counter++)
		{
			boidList[counter].Initialise(pRes, pScene);
		}

	}

	void Update(float frameUpdateTime, Scene* pScene) 
	{
		for (int counter = 0; counter <= NumBoids; counter++)
		{
			boidList[counter].ComputeForce(&boidList[0], pScene);
		}

		for (int counter = 0; counter <= NumBoids; counter++)
		{
			boidList[counter].Update(frameUpdateTime);
		}
	}




};




