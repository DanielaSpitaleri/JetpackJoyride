// ----------------------------------------------------------------
// From "Algorithms and Game Programming" in C++ by Alessandro Bria
// Copyright (C) 2024 Alessandro Bria (a.bria@unicas.it). 
// All rights reserved.
// 
// Released under the BSD License
// See LICENSE in root directory for full details.
// ----------------------------------------------------------------

#include "Barry.h" 
#include "SpriteFactory.h"
#include "Audio.h"
#include "PlatformerGame.h"
#include "Scene.h"
#include "Missle.h"
#include "LevelLoader.h"
#include "StaticObject.h"
#include "PlatformerGameScene.h"
#include "HUD.h"


using namespace agp;

Barry::Barry(Scene* scene, const PointF& pos, const PointF& terrain)
	: DynamicObject(scene, RectF( pos.x , pos.y-3.0f, 2.0f, 2.0f ), nullptr)
{
	_collider.adjust(0.2f, 0, -0.2, static_cast<float>(- 1.5f / 16.0f));
	_yGravityForce = 100;
	_rect = RectF(pos.x, -5, 4.0f, 4.0f);
	_walking = false;
	_jumping = false;
	_jumping2 = false;
	_running = false;
	_dying = false;
	_dying2 = false;
	_dead = false;
	_frozen = false;
	_bird = false;
	_invincible = false;
	addBird= false;
	profitBirdBlock= false;
	_activateBoost = false;
	_precBird = false;
	 _time = 0;
	 _timeSound = 0;
	 _timeSound2 = 0;
	 posXdie=0;
	_score = 0;
	_xLastNonZeroVel = 0;
	
	_sprites["walk"] = SpriteFactory::instance()->get("barry_walk");
	_sprites["jump1"] = SpriteFactory::instance()->get("barry_jump1");
	_sprites["jump2"] = SpriteFactory::instance()->get("barry_jump2");
	_sprites["die"] = SpriteFactory::instance()->get("barry_die");
	_sprites["die2"] = SpriteFactory::instance()->get("barry_die2");
	_sprites["fly1"] = SpriteFactory::instance()->get("bird_fly1");
	_sprites["fly2"] = SpriteFactory::instance()->get("bird_fly2");
	_sprites["bird_walk"] = SpriteFactory::instance()->get("bird_walk");

}

void Barry::update(float dt)
{
	PlatformerGameScene* platformerScene = dynamic_cast<PlatformerGameScene*>(_scene);
	Barry* barry = dynamic_cast<Barry*>(dynamic_cast<PlatformerGameScene*>(_scene)->player());
	
	if (_frozen)
		return;

	// physics
	DynamicObject::update(dt);
	std::list<Object*> potentialColliders = _scene->objects(sceneCollider().united(_rect));

	_time += dt;
	if (_time >= 0.25f && (!_dead && !_dying && !_dying2))
	{
		_score += 1;

		dynamic_cast<PlatformerGame*>(Game::instance())->hud()->setScore(_score);
		
		_time = 0.0f;
	}

	if (_bird) 
		_rect = RectF(_rect.pos.x, _rect.pos.y, 3.0f, 3.0f);
	 else 
		_rect = RectF(_rect.pos.x, _rect.pos.y, 2.0f, 2.0f); 
	
	if (_jumping && grounded() || _jumping2 && grounded()) 
	{
		_jumping = false;
		_jumping2 = false;
	}

	if (_vel.x != 0 && !_jumping)
		_xLastNonZeroVel = _vel.x;

	_walking = _vel.x != 0;
	_running = std::abs(_vel.x) > 6;

	// animations
	if (_dying)
		_sprite = _sprites["die"];
	else if (_dying2)
		_sprite = _sprites["die2"];
	
	else if (_jumping && !platformerScene->getResume()) 
	{
		if (_bird) 
			_sprite = _sprites["fly1"];
		else 
			_sprite = _sprites["jump1"];
		
	} else if (_jumping2 && !platformerScene->getResume()) 
	{
		if (_bird) 
			_sprite = _sprites["fly2"];
		else 
			_sprite = _sprites["jump2"];

	}else if (skidding()) 
	{
		if (_bird) 
			_sprite = _sprites["bird_walk"];
		else 
			_sprite = _sprites["skid"];
	
	}else if (_running) 
	{
		if (_bird)
			_sprite = _sprites["bird_walk"];
		else 
		_sprite = _sprites["run"];
			
	}else if (_walking)
	{
		if (_bird)
			_sprite = _sprites["bird_walk"];
		else 
			_sprite = _sprites["walk"];
	
	}else 
	{
		if (_bird) 
			_sprite = _sprites["bird_walk"];
		else
			_sprite = _sprites["stand"];
	}
	if ((_walking || _running || skidding()) && (!_jumping)&& !_jumping2 && !_bird) 
	{
		_timeSound += dt;
		if (_timeSound > _timeSound2) {
			Audio::instance()->playSound("run", 0);
			_timeSound2 = _timeSound + 0.2;
		}
	}
	
	for (Object* obj : potentialColliders) {
		Missle* missle = dynamic_cast<Missle*>(obj);
			Laser * laser = dynamic_cast<Laser*>(obj);
		if ( missle && missle->isMissle()) 
		{
			if (sceneCollider().intersects(missle->sceneCollider())) 
			{
				handleCollision(missle);
			}
		}
		else if (laser && laser->isLaser()) 
		{
			if (sceneCollider().intersects(laser->sceneCollider())) 
			{
				handleCollision(laser);
			}
		}
	}
}

void Barry::handleCollision(CollidableObject* obj)
{
	if (_bird) 
	{
		setBird(false);
		setActivateBoost(false);
		temporaryInvincibility();
		updateCollider();
	}else 
		die();
}

void Barry::updateCollider() 
{
	_collider.adjust(0, 0, -0.6f, -0.8f);
}

void Barry::activateBoost()
{
	_frozen = true;

	SpriteFactory* spriteLoader = SpriteFactory::instance();
	Sprite* profitBirdSprite = spriteLoader->get("profitBird");

	float posX = _rect.pos.x - 7.5f;
	float posY = -2;

	RectF profitBirdRect(posX, posY, 17, 3);
	StaticObject* profitBird = new StaticObject(_scene, profitBirdRect, profitBirdSprite, 2);

	_collider.adjust(0, 0, +0.6f, 0.8f);

	profitBirdBlock = true;

	_scene->schedule("add_profitBird", 0.0f, [this, profitBird]() 
	{
		_scene->newObject(profitBird);
	});

	_scene->schedule("remove_profitBird", 4.0f, [this, profitBird]() 
	{
		_bird = true;
		profitBirdBlock = false;

		_scene->killObject(profitBird);

		_frozen = false;
		dynamic_cast<PlatformerGame*>(Game::instance())->freeze(false);
	});

	setActivateBoost(true);

	_scene->schedule("remove_profitBird2", 20.0f, [this]() 
	{
		if (_bird) 
		{
			updateCollider();
			_bird = false;
			_precBird = true;
			setActivateBoost(false);

		}
	});
}

void Barry::move(Direction dir)
{
	if (_dying || _dead || _dying2)
		return;
	
	DynamicObject::move(dir);

}

void Barry::jump(bool on)
{
	static bool _spacePressed = false; 
	PlatformerGame* platformerGame = dynamic_cast<PlatformerGame*>(Game::instance());
	
	if (_dying || _dead || _dying2 || _frozen)
		return;

	if (on)
	{
		if (_bird)
		{
			if (!_spacePressed) 
			{
				velAdd(Vec2Df(0, -_yJumpImpulse));

				if (std::abs(_vel.x) < 9)
					_yGravityForce = 20;
				else
					_yGravityForce = 15;

				_jumping = true;
				_jumping2 = false;

				Audio::instance()->playSound("birdFlap2");

				_spacePressed = true; 
			}
		}
		else
		{
			velAdd(Vec2Df(0, -_yJumpImpulse));

			if (std::abs(_vel.x) < 9)
				_yGravityForce = 20;
			else
				_yGravityForce = 15;

			_jumping = true;
			_jumping2 = false;
		}
	}
	else
	{
		if (_bird && _spacePressed) 
			_spacePressed = false;
		
		if (!on && midair() && !_dying && !grounded() && !_dying2)
		{
			_yGravityForce = 60;
			_jumping2 = true;
			_jumping = false;
			_compenetrable = false;
		}
	}
}

void Barry::run(bool on)
{
	if (midair())
		return;

	if (on)
	{
		_xVelMax = 10;
		_xMoveForce = 13;
	}
	else
	{
		_xVelMax = 6;	
		_xMoveForce = 8;
	}

}

void Barry::die()
{
	if (!_invincible) {

		if (_dying || _dying2)
			return;

		PlatformerGame* game = dynamic_cast<PlatformerGame*>(Game::instance());

		_dying = true;
		_yGravityForce = 15.0f;
		_vel = { 0,0 };
		_xDir = Direction::NONE;
		Audio::instance()->haltMusic();
		Audio::instance()->playSound("electricity");
		
		dynamic_cast<PlatformerGame*>(Game::instance())->freeze(true);
		
		Vec2Df finalPos = { _rect.pos.x + 50.0f, 1 };       
		float duration = 1.0f;                    
		Vec2Df step = (finalPos - _rect.pos) / (duration * 60); 

		schedule("dying", 0.5f, [this,step, finalPos,game]()
		{
			velAdd(Vec2Df(0.0f, _yGravityForce));
			_rect.pos += _vel;

			if (_rect.pos.y >= 0) 
			{
				_dying2 = true;
				_dying = false;
				_rect.pos.y = 0; 
				_vel = { 0, 0 };
				_yGravityForce = 0;
				posXdie = _rect.pos.x;
				
				schedule("die", 2, [this,game]()
				{
					_dead = true;
					dynamic_cast<PlatformerGame*>(Game::instance())->gameover(game->hud()->getCoins());
				});
			}
		});

	}
}

void Barry::revive() {
	
	if (!_dead) return;
	Game::instance()->popSceneLater();

	_dead = false;
	_dying = false;
	_dying2 = false;
	_walking = false;
	_running = false;
	_jumping = false;
	_jumping2 = false;

	_rect.pos.x = posXdie;
	_rect.pos.y = -0.90625;
	_vel = { 0, 0 };

	_yGravityForce = 0;

	dynamic_cast<PlatformerGame*>(Game::instance())->freeze(false);

	temporaryInvincibility();
}

void Barry::temporaryInvincibility() 
{
	_invincible = true;

	schedule("invincibility_end", 0.5f, [this]() {
		_invincible = false;
	});
}