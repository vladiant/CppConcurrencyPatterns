# Strategized Locking

* Dealing with Mutation Pattern

* Enables it to use various locking strategies as replaceable
components.

* Is the application of the strategy pattern to locking.

## Idea

* You want to use your library in various domains.
* Depending on the domain, you want to use exclusive locking,
shared locking, or no locking.
* Configure your locking strategy at compile time or run time.

## Advantages

* Run-time polymorphism
   * Enables it to change the locking strategy during run time.
* Compile-time polymorphism
   * No cost at run time
   * Producer flatters object hierarchies

## Disadvantages

* Run-time polymorphism
   * Needs a pointer indirection.
* Compile-time polymorphism
   * Produces in the error case a quite challenging to understand error message (when no concepts are used)
