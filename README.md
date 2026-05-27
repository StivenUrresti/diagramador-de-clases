# Mini PlantUML En C

Interprete en C para crear diagramas de clases desde un DSL inspirado en el enunciado de Compiladores.

El flujo es:

```txt
archivo.uml -> lexer/parser -> modelo -> semantica -> layout -> plan_routes -> DOT intermedio -> SVG / ventana raylib
```

## Compilar

Compilacion base, sin dependencias graficas:

```bash
make
```

Compilacion con ventana propia y captura PNG, si tienes `raylib` instalada:

```bash
make raylib
```

## Ejecutar

Un solo comando: toma el `.uml`, genera `build/<nombre>.dot` y `build/<nombre>.svg`, y abre el visor:

```bash
make
./build/uml_engine examples/cuatro.uml --open
```

Con Make:

```bash
make diagram UML=examples/cuatro.uml
```

Tambien puedes usar el ejemplo por defecto:

```bash
make run
```

## Ejemplos Incluidos

Juego, basado en el ejemplo del PDF:

```bash
./build/uml_engine examples/juego.uml --open
```

Cuatro, diagrama minimo con cuatro clases y los cuatro tipos de relacion:

```bash
./build/uml_engine examples/cuatro.uml --open
```

Biblioteca, ejemplo con recursos, catalogo, estantes y libros:

```bash
./build/uml_engine examples/biblioteca.uml --open
```

Generar un archivo Graphviz DOT estandar con nodos UML y flechas renderizables:

```bash
./build/uml_engine examples/juego.uml --dot build/juego.dot --svg build/juego.svg
```

El `.dot` usa labels HTML de Graphviz y flechas estandar:

- `arrowhead=empty` para herencia.
- `arrowhead=open` y `style=dashed` para dependencia.
- `arrowhead=odiamond` para agregacion.
- `arrowhead=diamond` para composicion.

Ver una salida ASCII de depuracion:

```bash
./build/uml_engine examples/juego.uml --ascii
```

Abrir ventana propia y guardar PNG, compilando antes con `make raylib`:

```bash
./build/uml_engine examples/juego.uml --view --png build/juego.png
```

Si no pasas ninguna salida, el programa genera `diagrama.svg`.

## Gramatica Soportada

Un programa tiene tres secciones en este orden:

```txt
Diagrama nombre.

posicion Clase {
X=entero;
Y=entero;
}

segmento (ClaseA, ClaseB, [x1, y1, x2, y2, ...], relacion);

[abstracta] clase Nombre [relacion ClaseRelacionada] {
atributos:
visibilidad tipo id1, id2;
metodos:
visibilidad tipo nombre(tipo param);
}
```

Relaciones aceptadas:

- `herencia` y `hereda`
- `dependencia`
- `composicion`
- `agregacion`

Visibilidades aceptadas:

- `publico`, renderizado como `+`
- `privado`, renderizado como `-`
- `protegido`, renderizado como `#`

## Validaciones Semanticas

El analizador reporta errores cuando:

- Dos clases tienen el mismo nombre.
- Dos posiciones usan la misma coordenada `(X,Y)`.
- Una clase no tiene posicion.
- Una posicion apunta a una clase inexistente.
- Un segmento referencia clases inexistentes.
- Una relacion de clase apunta a una clase inexistente.
- Un atributo se repite en la misma clase.
- Un atributo se repite en una clase hija por herencia.
- Un atributo y un metodo tienen el mismo nombre.
- Dos metodos tienen la misma firma.

## Estructura

```txt
include/uml.h          API y estructuras compartidas
src/parser.c           lexer y parser recursivo descendente
src/semantic.c         validaciones y relaciones finales
src/layout.c           calculo de cajas UML y ASCII
src/route.c            planifica rutas ortogonales y evita cruces con cajas
src/render_dot.c       exportador DOT intermedio con posiciones y puntos
src/render_svg.c       exportador SVG
src/render_raylib.c    ventana propia opcional con raylib
src/main.c             CLI
examples/*.uml         ejemplos de diagramas y tipos de relacion
tests/duplicados.uml   ejemplo con errores semanticos
```
