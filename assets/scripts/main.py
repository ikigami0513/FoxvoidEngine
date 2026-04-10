from foxvoid import *
from typing import Optional


set_pixel_art_mode(True)


class PlayerController(Component):
    def __init__(self):
        super().__init__()
        self.speed = 400.0
        self._transform: Optional[Transform2d] = None

    def start(self):
        log("PlayerController start sequence!")
        self._transform = self.game_object.get_component(Transform2d)
        self._transform.scale = Vector2(4.0, 4.0)
        
    def update(self, delta_time: float):
        if self._transform is not None:
            if Input.is_key_down(Keys.RIGHT):
                self._transform.position.x += self.speed * delta_time
            if Input.is_key_down(Keys.LEFT):
                self._transform.position.x -= self.speed * delta_time
            
            if Input.is_key_down(Keys.UP):
                self._transform.position.y -= self.speed * delta_time
            if Input.is_key_down(Keys.DOWN):
                self._transform.position.y += self.speed * delta_time

            if Input.is_key_pressed(Keys.KEY_SPACE):
                log("my friend")

                friend = GameObject.spawn("Friend")

                transform = friend.add_component(Transform2d, self._transform.position.x + 20, self._transform.position.y + 20)
                transform.scale = Vector2(4.0, 4.0)
                friend.add_component(SpriteSheetRenderer, "assets/textures/player_base.png", 9, 56)                
                friend.add_component(Animation2d, [0, 1, 2, 3, 4, 5], 0.15, True)
