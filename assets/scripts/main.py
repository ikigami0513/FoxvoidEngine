from foxvoid import *
from typing import Optional

class PlayerController(Component):
    def __init__(self):
        super().__init__()

        self.speed = 400.0
        self.transform: Optional[Transform2d] = None

    def start(self):
        log("PlayerController start sequence!")
        self.transform = self.game_object.get_component(Transform2d)

    def update(self, delta_time: float):
        if self.transform is not None:
            # Handle movement (held keys)
            if Input.is_key_down(Keys.RIGHT):
                self.transform.x += self.speed * delta_time
            if Input.is_key_down(Keys.LEFT):
                self.transform.x -= self.speed * delta_time
            if Input.is_key_down(Keys.DOWN):
                self.transform.y += self.speed * delta_time
            if Input.is_key_down(Keys.UP):
                self.transform.y -= self.speed * delta_time

            if Input.is_key_pressed(Keys.SPACE):
                log("Space key pressed! Piou piou!")
                self.transform.y -= 50.0
