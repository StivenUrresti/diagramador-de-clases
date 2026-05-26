# Requerimientos — Mini PlantUML / Intérprete de Diagramas UML

## Objetivo General

Desarrollar un intérprete capaz de leer un archivo escrito en un lenguaje propio (DSL) para diagramas UML y generar automáticamente un diagrama visual de clases sin depender de herramientas externas como PlantUML.

El sistema deberá:

- Leer archivos de texto con sintaxis propia.
- Interpretar clases, atributos, métodos y relaciones.
- Validar reglas semánticas.
- Generar automáticamente un diagrama UML.
- Exportar el resultado visual como imagen o SVG.

---

# Arquitectura General del Sistema

```txt
Archivo DSL
    ↓
Lexer
    ↓
Parser
    ↓
AST (Árbol Sintáctico)
    ↓
Analizador Semántico
    ↓
Renderizador UML
    ↓
SVG / PNG / Consola
```

---

# Requerimientos Funcionales

## RF-01 — Lectura de archivos

El sistema debe permitir cargar archivos `.txt` o `.uml`.

Ejemplo:

```txt
diagrama juego.
```

---

## RF-02 — Análisis Léxico (Lexer)

El sistema debe convertir el texto de entrada en tokens válidos.

Debe reconocer:

- Palabras reservadas
- Identificadores
- Tipos de datos
- Símbolos
- Coordenadas
- Relaciones
- Visibilidad

### Ejemplo de tokens

```txt
clase -> KEYWORD
Usuario -> IDENTIFIER
{ -> LBRACE
```

---

## RF-03 — Análisis Sintáctico (Parser)

El sistema debe validar la estructura gramatical del lenguaje.

Debe interpretar:

- Clases
- Métodos
- Atributos
- Relaciones
- Posiciones
- Segmentos

---

## RF-04 — Construcción del AST

El sistema debe construir una representación interna del diagrama mediante nodos.

### Ejemplo

```txt
DiagramNode
 ├── ClassNode
 ├── AttributeNode
 ├── MethodNode
 └── RelationNode
```

---

## RF-05 — Manejo de clases

El sistema debe permitir declarar clases.

### Ejemplo

```txt
clase Usuario {
}
```

---

## RF-06 — Manejo de atributos

El sistema debe soportar atributos con:

- Tipo
- Visibilidad
- Nombre

### Ejemplo

```txt
privado string nombre;
```

---

## RF-07 — Manejo de métodos

El sistema debe soportar métodos con:

- Visibilidad
- Tipo de retorno
- Nombre
- Parámetros

### Ejemplo

```txt
publico void login(string correo);
```

---

## RF-08 — Manejo de relaciones UML

El sistema debe soportar:

- Herencia
- Dependencia
- Agregación
- Composición

### Ejemplo

```txt
clase Admin herencia Usuario {
}
```

---

## RF-09 — Manejo de posiciones

Cada clase debe poder declararse con coordenadas X,Y.

### Ejemplo

```txt
posicion Usuario {
 X=100;
 Y=200;
}
```

---

## RF-10 — Generación automática del diagrama

El sistema debe dibujar automáticamente:

- Rectángulos UML
- Divisiones internas
- Relaciones
- Flechas
- Rombos
- Texto

---

## RF-11 — Exportación visual

El sistema debe permitir exportar el resultado como:

- SVG
- PNG (opcional)

---

## RF-12 — Vista en consola

El sistema debe permitir una representación ASCII del diagrama para debugging.

### Ejemplo

```txt
+----------------+
| Usuario        |
+----------------+
| - nombre       |
+----------------+
| + login()      |
+----------------+
```

---

# Requerimientos Semánticos

## RS-01 — Nombres únicos

No pueden existir dos clases con el mismo nombre.

---

## RS-02 — Posiciones únicas

No pueden existir dos clases en la misma posición X,Y.

---

## RS-03 — Validación de herencia

Una clase padre debe existir antes de ser referenciada.

---

## RS-04 — Validación de atributos duplicados

No se pueden repetir atributos dentro de la misma clase.

---

## RS-05 — Validación de métodos duplicados

No se pueden repetir métodos con la misma firma.

---

## RS-06 — Validación de referencias

Toda relación debe apuntar a clases existentes.

---

# Requerimientos del Renderizador UML

## RR-01 — Renderizado de clases

Cada clase debe representarse como un rectángulo dividido en:

1. Nombre
2. Atributos
3. Métodos

---

## RR-02 — Visibilidad UML

Representación:

| Visibilidad | Símbolo |
|---|---|
| publico | + |
| privado | - |
| protegido | # |

---

## RR-03 — Renderizado de relaciones

| Relación | Representación |
|---|---|
| Herencia | Flecha triangular |
| Dependencia | Línea punteada |
| Agregación | Rombo vacío |
| Composición | Rombo relleno |

---

# Requerimientos Técnicos

## RT-01 — Lenguaje sugerido

Opciones recomendadas:

- Java
- TypeScript
- Python

---

## RT-02 — Formato de salida recomendado

SVG como formato principal.

Ventajas:

- Escalable
- Fácil de generar
- Compatible con navegadores
- Convertible a PNG

---

## RT-03 — Arquitectura modular

El proyecto debe separarse en módulos:

```txt
lexer/
parser/
semantic/
renderer/
models/
utils/
```

---

# Requerimientos No Funcionales

## RNF-01 — Escalabilidad

El sistema debe permitir agregar nuevos tipos de relaciones o diagramas en el futuro.

---

## RNF-02 — Mantenibilidad

El código debe estar desacoplado entre parsing y renderizado.

---

## RNF-03 — Rendimiento

El sistema debe generar diagramas en menos de 5 segundos para archivos medianos.

---

## RNF-04 — Portabilidad

Debe poder ejecutarse desde consola.

### Ejemplo

```bash
uml-engine archivo.uml
```

---

# Flujo esperado del sistema

```txt
archivo.uml
    ↓
Lexer
    ↓
Parser
    ↓
AST
    ↓
Semantic Analyzer
    ↓
SVG Renderer
    ↓
diagrama.svg
```

---

# Entregables

## Entregable 1

- Lexer funcional
- Reconocimiento de tokens

---

## Entregable 2

- Parser funcional
- Construcción del AST

---

## Entregable 3

- Validaciones semánticas

---

## Entregable 4

- Generación ASCII

---

## Entregable 5

- Generación SVG

---

## Entregable 6

- Exportación final de diagramas

---

# Posibles mejoras futuras

- Interfaz gráfica drag & drop
- Editor visual
- Exportación PDF
- Soporte para más diagramas UML
- Zoom y navegación
- Generación automática de código
- Importación desde código fuente
