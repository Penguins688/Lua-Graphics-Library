# Lua Graphics Library

![warning](https://img.shields.io/badge/DISCLAIMER-Executable%20only%20works%20on%20macOS-red)  
![compile](https://img.shields.io/badge/Compile%20yourself%20if%20using%20a%20different%20OS-yellow) 

## Requirements

- **Lua:** You need to have lua installed

## Running

**Make sure you have a main.lua file with the update function in your src directory**

**To run the project enter this command:**

```console
./run
```

## Documentation

### Windows

**Creating a window is very simple**

```lua
window = Window("Title", <width>, <height>)
```

**Make sure you always set the window to a global variable**

### Rectangles and Circles

**To Draw a rectangle use the DrawRect Runction**

```lua
window = Window("Example", 800, 600)

function update()
  DrawRect(<x>, <y>, <width>, <height>, <r>, <g>, <b>, <a>)
end
```

**Circles are the same except you use the DrawCircle function**

```lua
window = Window("Example", 800, 600)

function update()
  DrawCircle(<x>, <y>, <radius>, <r>, <g>, <b>, <a>)
end
```

### Images

**To draw an image use the DrawTexture function**

```lua
  window = Window("Example", 800, 600)
  Texture = LoadTexture("src/<image>", window)
  
  function update()
      DrawTexture(Texture, window, <x>, <y>)
  end
```

**To draw an image with a custom width just add it**

```lua
  window = Window("Example", 800, 600)
  Texture = LoadTexture("src/<image>", window)
  
  function update()
      DrawTexture(Texture, window, <x>, <y>, <width>, <height>)
  end
```

**To rotate the texture draw it with the RotateTexture function**

```lua
  window = Window("Example", 800, 600)
  Texture = LoadTexture("src/<image>", window)
  
  function update()
      DrawTexture(Texture, window, <angle>)
  end
```

### Fonts

**To load a font use the LoadFont function**

```lua
font = LoadFont(src/<font>, <font-size>)
```

**To draw it use the draw font function**

```lua
window = Window("Example", 800, 600)
font = LoadFont(src/font.ttf, 30)

function update()
  DrawFont(font, window, <text>, <x>, <y>, <r>, <g>, <b>)
end
```

### Sounds

**To play a sound use the PlaySound function**

```lua
window = Window("Example", 800, 600)
PlaySound(src/<sound>, false) -- false for no looping and true for indefinite looping

function update()
  
end
```

## Changelog

- **1.0**: Initial commit.
