from foxvoid import *
from typing import Optional


set_pixel_art_mode(True)


class PlayerController(Component):
    def __init__(self):
        super().__init__()
        self.speed = 400.0
        self.jump_force = -500.0
        # self.msg = "hello hot reloading"
        self._transform: Optional[Transform2d] = None
        self._rigidbody: Optional[RigidBody2d] = None

    def start(self):
        Debug.log("PlayerController start sequence!")
        self._transform = self.game_object.get_component(Transform2d)
        self._rigidbody = self.game_object.get_component(RigidBody2d)
        
    def update(self, delta_time: float):
        if self._transform is not None:
            if Input.is_key_down(Keys.RIGHT):
                self._transform.position.x += self.speed * delta_time
            if Input.is_key_down(Keys.LEFT):
                self._transform.position.x -= self.speed * delta_time

            if Input.is_key_pressed(Keys.KEY_SPACE) and self._rigidbody.is_grounded:
                self._rigidbody.velocity.y = self.jump_force

    def on_collision(self, collision: Collision2D):
        if collision.other is not None:
            Debug.log(f"Just it an object: {collision.other.name}")
        else:
            if collision.normal.y == -1.0:
                Debug.log("hit the ground")
            elif collision.normal.x != 0.0:
                Debug.log("hit the wall")
