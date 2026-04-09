from foxvoid import *
from typing import Optional


set_pixel_art_mode(True)


class PlayerController(Component):
    def __init__(self):
        super().__init__()
        self.speed = 400.0
        self.transform: Optional[Transform2d] = None
        self.sprite: Optional[SpriteSheetRenderer] = None
        
        # Custom logic variables for animation
        self.animation_timer = 0.0
        self.animation_speed = 0.1 # Change frame every 0.1 seconds

    def start(self):
        log("PlayerController start sequence!")
        self.transform = self.game_object.get_component(Transform2d)
        self.transform.scale = Vector2(4.0, 4.0)
        
        # Add the Spritesheet
        self.game_object.add_component(SpriteSheetRenderer, "assets/textures/player_base.png", 9, 56)
        self.game_object.add_component(Animation2d, [0, 1, 2, 3, 4, 5], 0.15, True)

    def update(self, delta_time: float):
        if self.transform is not None:
            if Input.is_key_down(Keys.RIGHT):
                self.transform.position.x += self.speed * delta_time
            if Input.is_key_down(Keys.LEFT):
                self.transform.position.x -= self.speed * delta_time
            
            if Input.is_key_down(Keys.UP):
                self.transform.position.y -= self.speed * delta_time
            if Input.is_key_down(Keys.DOWN):
                self.transform.position.y += self.speed * delta_time

            if Input.is_key_pressed(Keys.KEY_SPACE):
                log("my friend")

                friend = GameObject.instantiate("Friend")

                transform = friend.add_component(Transform2d, self.transform.position.x + 20, self.transform.position.y + 20)
                transform.scale = Vector2(4.0, 4.0)
                friend.add_component(SpriteSheetRenderer, "assets/textures/player_base.png", 9, 56)                
                friend.add_component(Animation2d, [0, 1, 2, 3, 4, 5], 0.15, True)
