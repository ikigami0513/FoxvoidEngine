from foxvoid import *
from typing import Optional


set_pixel_art_mode(True)


class PlayerController(Component):
    def __init__(self):
        super().__init__()
        self.speed = 400.0
        self.jump_force = -500.0

        # State tracker to remember which way the player is looking
        self.facing_right = True

        self._transform: Optional[Transform2d] = None
        self._rigidbody: Optional[RigidBody2d] = None
        self._animator: Optional[Animator2d] = None

    def start(self):
        Debug.log("PlayerController start sequence!")
        self._transform = self.game_object.get_component(Transform2d)
        self._rigidbody = self.game_object.get_component(RigidBody2d)
        self._animator = self.game_object.get_component(Animator2d)
        
    def update(self, delta_time: float):
        # Ensure all components are loaded before executing logic
        if self._transform is None or self._rigidbody is None or self._animator is None:
            return
        
        is_moving = False

        # Handle movement and flip state
        if Input.is_key_down(Keys.RIGHT):
            self._transform.position.x += self.speed * delta_time
            self.facing_right = True
            is_moving = True
            
        elif Input.is_key_down(Keys.LEFT):
            self._transform.position.x -= self.speed * delta_time
            self.facing_right = False
            is_moving = True

        # Handle jumping
        if Input.is_key_pressed(Keys.KEY_SPACE) and self._rigidbody.is_grounded:
            self._rigidbody.velocity.y = self.jump_force

        # Resolve animations based on current state
        if is_moving:
            if self.facing_right:
                self._animator.play("walk_right")
            else:
                self._animator.play("walk_left")
        else:
            if self.facing_right:
                self._animator.play("idle_right")
            else:
                self._animator.play("idle_left")

    def on_collision(self, collision: Collision2D):
        if collision.other is not None:
            Debug.log(f"Just it an object: {collision.other.name}")
