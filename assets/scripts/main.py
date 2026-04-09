from foxvoid import *
from typing import Optional

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
        
        # Add the Spritesheet
        self.sprite = self.game_object.add_component(SpriteSheetRenderer, "assets/textures/player_base.png", 9, 56)

    def update(self, delta_time: float):
        # 1. Handle Animation
        if self.sprite is not None:
            self.animation_timer += delta_time
            if self.animation_timer >= self.animation_speed:
                self.animation_timer = 0.0
                
                # Loop to the next frame!
                next_frame = self.sprite.frame + 1
                if next_frame >= self.sprite.frame_count:
                    next_frame = 0
                    
                self.sprite.frame = next_frame

        # 2. Handle Movement
        if self.transform is not None:
            if Input.is_key_down(Keys.RIGHT):
                self.transform.x += self.speed * delta_time
            if Input.is_key_down(Keys.LEFT):
                self.transform.x -= self.speed * delta_time
