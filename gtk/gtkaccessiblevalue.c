/* gtkaccessiblevalue.c: Accessible value
 *
 * Copyright 2020  GNOME Foundation
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

/*< private >
 * SECTION: gtkaccessiblevalue
 * @Title: GtkAccessibleValue
 * @Short_description: Container for accessible state and property values
 *
 * GtkAccessibleValue is a reference counted, generic container for values used
 * to represent the state and properties of a #GtkAccessible implementation.
 *
 * There are two kinds of accessible value types:
 *
 *  - hard coded, static values; GTK owns them, and their contents, and are
 *    guaranteed to exist for the duration of the application's life time
 *  - dynamic values; the accessible state owns the value and their contents,
 *    and they can be allocated and freed
 *
 * Typically, the former type of values is used for boolean, tristate, and
 * token value; the latter is used for numbers, strings, and token lists.
 *
 * For more information on the types of values, see the [WAI-ARIA](https://www.w3.org/WAI/PF/aria/states_and_properties#propcharacteristic_value)
 * reference.
 */

#include "config.h"

#include "gtkaccessiblevalueprivate.h"

#include "gtkaccessible.h"
#include "gtkenums.h"

G_DEFINE_QUARK (gtk-accessible-value-error-quark, gtk_accessible_value_error)

G_DEFINE_BOXED_TYPE (GtkAccessibleValue, gtk_accessible_value,
                     gtk_accessible_value_ref,
                     gtk_accessible_value_unref)

/*< private >
 * gtk_accessible_value_alloc:
 * @value_class: a #GtkAccessibleValueClass structure
 *
 * Allocates a new #GtkAccessibleValue subclass using @value_class as the
 * type definition.
 *
 * Returns: (transfer full): the newly allocated #GtkAccessibleValue
 */
GtkAccessibleValue *
gtk_accessible_value_alloc (const GtkAccessibleValueClass *value_class)
{
  g_return_val_if_fail (value_class != NULL, NULL);
  g_return_val_if_fail (value_class->instance_size >= sizeof (GtkAccessibleValue), NULL);

  GtkAccessibleValue *res = g_malloc0 (value_class->instance_size);

  /* We do not use grefcount, here, because we want to have statically
   * allocated GtkAccessibleValue subclasses, and those cannot be initialized
   * with g_ref_count_init()
   */
  res->ref_count = 1;
  res->value_class = value_class;

  if (res->value_class->init != NULL)
    res->value_class->init (res);

  return res;
}

/*< private >
 * gtk_accessible_value_ref:
 * @self: a #GtkAccessibleValue
 *
 * Acquires a reference on the given #GtkAccessibleValue.
 *
 * Returns: (transfer full): the value, with an additional reference
 */
GtkAccessibleValue *
gtk_accessible_value_ref (GtkAccessibleValue *self)
{
  g_return_val_if_fail (self != NULL, NULL);

  self->ref_count += 1;

  return self;
}

/*< private >
 * gtk_accessible_value_unref:
 * @self: (transfer full): a #GtkAccessibleValue
 *
 * Releases a reference on the given #GtkAccessibleValue.
 */
void
gtk_accessible_value_unref (GtkAccessibleValue *self)
{
  g_return_if_fail (self != NULL);

  self->ref_count -= 1;
  if (self->ref_count == 0)
    {
      if (self->value_class->finalize != NULL)
        self->value_class->finalize (self);

      g_free (self);
    }
}

/*< private >
 * gtk_accessible_value_print:
 * @self: a #GtkAccessibleValue
 * @buffer: a #GString
 *
 * Prints the contents of a #GtkAccessibleValue into the given @buffer.
 */
void
gtk_accessible_value_print (const GtkAccessibleValue *self,
                            GString                  *buffer)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (buffer != NULL);

  if (self->value_class->print != NULL)
    self->value_class->print (self, buffer);
}

/*< private >
 * gtk_accessible_value_to_string:
 * @self: a #GtkAccessibleValue
 *
 * Fills a string with the contents of the given #GtkAccessibleValue.
 *
 * Returns: (transfer full): a string with the contents of the value
 */
char *
gtk_accessible_value_to_string (const GtkAccessibleValue *self)
{
  g_return_val_if_fail (self != NULL, NULL);

  GString *buffer = g_string_new (NULL);

  gtk_accessible_value_print (self, buffer);

  return g_string_free (buffer, FALSE);
}

/*< private >
 * gtk_accessible_value_equal:
 * @value_a: (nullable): the first #GtkAccessibleValue
 * @value_b: (nullable): the second #GtkAccessibleValue
 *
 * Checks whether @value_a and @value_b are equal.
 *
 * This function is %NULL-safe.
 *
 * Returns: %TRUE if the given #GtkAccessibleValue instances are equal,
 *   and %FALSE otherwise
 */
gboolean
gtk_accessible_value_equal (const GtkAccessibleValue *value_a,
                            const GtkAccessibleValue *value_b)
{
  if (value_a == value_b)
    return TRUE;

  if (value_a == NULL || value_b == NULL)
    return FALSE;

  return value_a->value_class->equal (value_a, value_b);
}

/* {{{ Basic types */

typedef struct {
  GtkAccessibleValue parent;

  int value;
} GtkIntAccessibleValue;

static gboolean
gtk_int_accessible_value_equal (const GtkAccessibleValue *value_a,
                                const GtkAccessibleValue *value_b)
{
  const GtkIntAccessibleValue *self_a = (GtkIntAccessibleValue *) value_a;
  const GtkIntAccessibleValue *self_b = (GtkIntAccessibleValue *) value_b;

  return self_a->value == self_b->value;
}

static void
gtk_int_accessible_value_print (const GtkAccessibleValue *value,
                                GString                  *buffer)
{
  const GtkIntAccessibleValue *self = (GtkIntAccessibleValue *) value;

  g_string_append_printf (buffer, "%d", self->value);
}

static const GtkAccessibleValueClass GTK_INT_ACCESSIBLE_VALUE = {
  .type_name = "GtkIntAccessibleValue",
  .instance_size = sizeof (GtkIntAccessibleValue),
  .equal = gtk_int_accessible_value_equal,
  .print = gtk_int_accessible_value_print,
};

GtkAccessibleValue *
gtk_int_accessible_value_new (int value)
{
  GtkAccessibleValue *res = gtk_accessible_value_alloc (&GTK_INT_ACCESSIBLE_VALUE);

  /* XXX: Possible optimization: statically allocate the first N values
   *      and hand out references to them, instead of dynamically
   *      allocating a new GtkAccessibleValue instance. Needs some profiling
   *      to figure out the common integer values used by large applications
   */
  GtkIntAccessibleValue *self = (GtkIntAccessibleValue *) res;
  self->value = value;

  return res;
}

int
gtk_int_accessible_value_get (const GtkAccessibleValue *value)
{
  GtkIntAccessibleValue *self = (GtkIntAccessibleValue *) value;

  g_return_val_if_fail (value != NULL, 0);
  g_return_val_if_fail (value->value_class == &GTK_INT_ACCESSIBLE_VALUE, 0);

  return self->value;
}

typedef struct {
  GtkAccessibleValue parent;

  double value;
} GtkNumberAccessibleValue;

static gboolean
gtk_number_accessible_value_equal (const GtkAccessibleValue *value_a,
                                   const GtkAccessibleValue *value_b)
{
  const GtkNumberAccessibleValue *self_a = (GtkNumberAccessibleValue *) value_a;
  const GtkNumberAccessibleValue *self_b = (GtkNumberAccessibleValue *) value_b;

  return G_APPROX_VALUE (self_a->value, self_b->value, 0.001);
}

static void
gtk_number_accessible_value_print (const GtkAccessibleValue *value,
                                   GString                  *buffer)
{
  const GtkNumberAccessibleValue *self = (GtkNumberAccessibleValue *) value;

  g_string_append_printf (buffer, "%g", self->value);
}

static const GtkAccessibleValueClass GTK_NUMBER_ACCESSIBLE_VALUE = {
  .type_name = "GtkNumberAccessibleValue",
  .instance_size = sizeof (GtkNumberAccessibleValue),
  .equal = gtk_number_accessible_value_equal,
  .print = gtk_number_accessible_value_print,
};

GtkAccessibleValue *
gtk_number_accessible_value_new (double value)
{
  GtkAccessibleValue *res = gtk_accessible_value_alloc (&GTK_NUMBER_ACCESSIBLE_VALUE);

  GtkNumberAccessibleValue *self = (GtkNumberAccessibleValue *) res;
  self->value = value;

  return res;
}

double
gtk_number_accessible_value_get (const GtkAccessibleValue *value)
{
  GtkNumberAccessibleValue *self = (GtkNumberAccessibleValue *) value;

  g_return_val_if_fail (value != NULL, 0);
  g_return_val_if_fail (value->value_class == &GTK_NUMBER_ACCESSIBLE_VALUE, 0);

  return self->value;
}

typedef struct {
  GtkAccessibleValue parent;

  char *value;
  gsize length;
} GtkStringAccessibleValue;

static void
gtk_string_accessible_value_finalize (GtkAccessibleValue *value)
{
  GtkStringAccessibleValue *self = (GtkStringAccessibleValue *) value;

  g_free (self->value);
}

static gboolean
gtk_string_accessible_value_equal (const GtkAccessibleValue *value_a,
                                   const GtkAccessibleValue *value_b)
{
  const GtkStringAccessibleValue *self_a = (GtkStringAccessibleValue *) value_a;
  const GtkStringAccessibleValue *self_b = (GtkStringAccessibleValue *) value_b;

  if (self_a->length != self_b->length)
    return FALSE;

  return g_strcmp0 (self_a->value, self_b->value) == 0;
}

static void
gtk_string_accessible_value_print (const GtkAccessibleValue *value,
                                   GString                  *buffer)
{
  const GtkStringAccessibleValue *self = (GtkStringAccessibleValue *) value;

  g_string_append (buffer, self->value);
}

static const GtkAccessibleValueClass GTK_STRING_ACCESSIBLE_VALUE = {
  .type_name = "GtkStringAccessibleValue",
  .instance_size = sizeof (GtkStringAccessibleValue),
  .finalize = gtk_string_accessible_value_finalize,
  .equal = gtk_string_accessible_value_equal,
  .print = gtk_string_accessible_value_print,
};

GtkAccessibleValue *
gtk_string_accessible_value_new (const char *str)
{
  GtkAccessibleValue *res = gtk_accessible_value_alloc (&GTK_STRING_ACCESSIBLE_VALUE);

  GtkStringAccessibleValue *self = (GtkStringAccessibleValue *) res;

  self->value = g_strdup (str);
  self->length = strlen (str);

  return res;
}

const char *
gtk_string_accessible_value_get (const GtkAccessibleValue *value)
{
  GtkStringAccessibleValue *self = (GtkStringAccessibleValue *) value;

  g_return_val_if_fail (value != NULL, 0);
  g_return_val_if_fail (value->value_class == &GTK_STRING_ACCESSIBLE_VALUE, 0);

  return self->value;
}

typedef struct {
  GtkAccessibleValue parent;

  GtkAccessible *ref;
} GtkReferenceAccessibleValue;

static void
remove_weak_ref (gpointer  data,
                 GObject  *old_reference)
{
  GtkReferenceAccessibleValue *self = data;

  self->ref = NULL;
}

static void
gtk_reference_accessible_value_finalize (GtkAccessibleValue *value)
{
  GtkReferenceAccessibleValue *self = (GtkReferenceAccessibleValue *) value;

  if (self->ref != NULL)
    g_object_weak_unref (G_OBJECT (self->ref), remove_weak_ref, self);
}

static gboolean
gtk_reference_accessible_value_equal (const GtkAccessibleValue *value_a,
                                      const GtkAccessibleValue *value_b)
{
  const GtkReferenceAccessibleValue *self_a = (GtkReferenceAccessibleValue *) value_a;
  const GtkReferenceAccessibleValue *self_b = (GtkReferenceAccessibleValue *) value_b;

  return self_a->ref == self_b->ref;
}

static void
gtk_reference_accessible_value_print (const GtkAccessibleValue *value,
                                      GString                  *buffer)
{
  const GtkReferenceAccessibleValue *self = (GtkReferenceAccessibleValue *) value;

  if (self->ref != NULL)
    {
      g_string_append_printf (buffer, "%s<%p>",
                              G_OBJECT_TYPE_NAME (self->ref),
                              self->ref);
    }
  else
    {
      g_string_append (buffer, "<null>");
    }
}

static const GtkAccessibleValueClass GTK_REFERENCE_ACCESSIBLE_VALUE = {
  .type_name = "GtkReferenceAccessibleValue",
  .instance_size = sizeof (GtkReferenceAccessibleValue),
  .finalize = gtk_reference_accessible_value_finalize,
  .equal = gtk_reference_accessible_value_equal,
  .print = gtk_reference_accessible_value_print,
};

GtkAccessibleValue *
gtk_reference_accessible_value_new (GtkAccessible *ref)
{
  g_return_val_if_fail (GTK_IS_ACCESSIBLE (ref), NULL);

  GtkAccessibleValue *res = gtk_accessible_value_alloc (&GTK_REFERENCE_ACCESSIBLE_VALUE);

  GtkReferenceAccessibleValue *self = (GtkReferenceAccessibleValue *) res;

  self->ref = ref;
  g_object_weak_ref (G_OBJECT (self->ref), remove_weak_ref, self);

  return res;
}

GtkAccessible *
gtk_reference_accessible_value_get (const GtkAccessibleValue *value)
{
  GtkReferenceAccessibleValue *self = (GtkReferenceAccessibleValue *) value;

  g_return_val_if_fail (value != NULL, 0);
  g_return_val_if_fail (value->value_class == &GTK_REFERENCE_ACCESSIBLE_VALUE, 0);

  return self->ref;
}

/* }}} */

/* {{{ Collection API */

typedef enum {
  GTK_ACCESSIBLE_COLLECT_INVALID,
  GTK_ACCESSIBLE_COLLECT_BOOLEAN,
  GTK_ACCESSIBLE_COLLECT_INT,
  GTK_ACCESSIBLE_COLLECT_TRISTATE,
  GTK_ACCESSIBLE_COLLECT_ENUM,
  GTK_ACCESSIBLE_COLLECT_NUMBER,
  GTK_ACCESSIBLE_COLLECT_STRING,
  GTK_ACCESSIBLE_COLLECT_REF
} GtkAccessibleCollectType;

typedef struct {
  int value;
  GtkAccessibleCollectType ctype;
  const char *name;
  GCallback ctor;
} GtkAccessibleCollect;

static const GtkAccessibleCollect collect_states[] = {
/* | State                         | Collected type                 | Name      | Constructor                      |
 * |-------------------------------|--------------------------------|-----------|----------------------------------|
 */
  { GTK_ACCESSIBLE_STATE_BUSY,     GTK_ACCESSIBLE_COLLECT_BOOLEAN,  "busy",     (GCallback) gtk_boolean_accessible_value_new },
  { GTK_ACCESSIBLE_STATE_CHECKED,  GTK_ACCESSIBLE_COLLECT_ENUM,     "checked",  (GCallback) gtk_checked_accessible_value_new },
  { GTK_ACCESSIBLE_STATE_DISABLED, GTK_ACCESSIBLE_COLLECT_BOOLEAN,  "disabled", (GCallback) gtk_boolean_accessible_value_new },
  { GTK_ACCESSIBLE_STATE_EXPANDED, GTK_ACCESSIBLE_COLLECT_TRISTATE, "expanded", (GCallback) gtk_expanded_accessible_value_new },
  { GTK_ACCESSIBLE_STATE_GRABBED,  GTK_ACCESSIBLE_COLLECT_TRISTATE, "grabbed",  (GCallback) gtk_grabbed_accessible_value_new },
  { GTK_ACCESSIBLE_STATE_HIDDEN,   GTK_ACCESSIBLE_COLLECT_BOOLEAN,  "hidden",   (GCallback) gtk_boolean_accessible_value_new },
  { GTK_ACCESSIBLE_STATE_INVALID,  GTK_ACCESSIBLE_COLLECT_ENUM,     "invalid",  (GCallback) gtk_invalid_accessible_value_new },
  { GTK_ACCESSIBLE_STATE_PRESSED,  GTK_ACCESSIBLE_COLLECT_ENUM,     "pressed",  (GCallback) gtk_pressed_accessible_value_new },
  { GTK_ACCESSIBLE_STATE_SELECTED, GTK_ACCESSIBLE_COLLECT_TRISTATE, "selected", (GCallback) gtk_selected_accessible_value_new },
};

static const GtkAccessibleCollect collect_props[] = {
  { GTK_ACCESSIBLE_PROPERTY_ACTIVE_DESCENDANT, GTK_ACCESSIBLE_COLLECT_REF,     "activedescendant", (GCallback) gtk_reference_accessible_value_new },
  { GTK_ACCESSIBLE_PROPERTY_AUTOCOMPLETE,      GTK_ACCESSIBLE_COLLECT_ENUM,    "autocomplete",     (GCallback) gtk_autocomplete_accessible_value_new },
  { GTK_ACCESSIBLE_PROPERTY_CONTROLS,          GTK_ACCESSIBLE_COLLECT_REF,     "controls",         (GCallback) gtk_reference_accessible_value_new },
  { GTK_ACCESSIBLE_PROPERTY_DESCRIBED_BY,      GTK_ACCESSIBLE_COLLECT_REF,     "describedby",      (GCallback) gtk_reference_accessible_value_new },
  { GTK_ACCESSIBLE_PROPERTY_FLOW_TO,           GTK_ACCESSIBLE_COLLECT_REF,     "flowto",           (GCallback) gtk_reference_accessible_value_new },
  { GTK_ACCESSIBLE_PROPERTY_HAS_POPUP,         GTK_ACCESSIBLE_COLLECT_BOOLEAN, "haspopup",         (GCallback) gtk_boolean_accessible_value_new },
  { GTK_ACCESSIBLE_PROPERTY_LABEL,             GTK_ACCESSIBLE_COLLECT_STRING,  "label",            (GCallback) gtk_string_accessible_value_new },
  { GTK_ACCESSIBLE_PROPERTY_LABELLED_BY,       GTK_ACCESSIBLE_COLLECT_REF   ,  "labelledby",       (GCallback) gtk_reference_accessible_value_new },
  { GTK_ACCESSIBLE_PROPERTY_LEVEL,             GTK_ACCESSIBLE_COLLECT_INT,     "level",            (GCallback) gtk_int_accessible_value_new },
  { GTK_ACCESSIBLE_PROPERTY_MULTI_LINE,        GTK_ACCESSIBLE_COLLECT_BOOLEAN, "multiline",        (GCallback) gtk_boolean_accessible_value_new },
  { GTK_ACCESSIBLE_PROPERTY_MULTI_SELECTABLE,  GTK_ACCESSIBLE_COLLECT_BOOLEAN, "multiselectable",  (GCallback) gtk_boolean_accessible_value_new },
  { GTK_ACCESSIBLE_PROPERTY_ORIENTATION,       GTK_ACCESSIBLE_COLLECT_ENUM,    "orientation",      (GCallback) gtk_orientation_accessible_value_new },
  { GTK_ACCESSIBLE_PROPERTY_OWNS,              GTK_ACCESSIBLE_COLLECT_REF,     "owns",             (GCallback) gtk_reference_accessible_value_new },
  { GTK_ACCESSIBLE_PROPERTY_POS_IN_SET,        GTK_ACCESSIBLE_COLLECT_INT,     "posinset",         (GCallback) gtk_int_accessible_value_new },
  { GTK_ACCESSIBLE_PROPERTY_READ_ONLY,         GTK_ACCESSIBLE_COLLECT_BOOLEAN, "readonly",         (GCallback) gtk_boolean_accessible_value_new },
  { GTK_ACCESSIBLE_PROPERTY_RELEVANT,          GTK_ACCESSIBLE_COLLECT_STRING,  "relevant",         (GCallback) gtk_string_accessible_value_new },
  { GTK_ACCESSIBLE_PROPERTY_REQUIRED,          GTK_ACCESSIBLE_COLLECT_BOOLEAN, "required",         (GCallback) gtk_boolean_accessible_value_new },
  { GTK_ACCESSIBLE_PROPERTY_SET_SIZE,          GTK_ACCESSIBLE_COLLECT_INT,     "setsize",          (GCallback) gtk_int_accessible_value_new },
  { GTK_ACCESSIBLE_PROPERTY_SORT,              GTK_ACCESSIBLE_COLLECT_ENUM,    "sort",             (GCallback) gtk_sort_accessible_value_new },
  { GTK_ACCESSIBLE_PROPERTY_VALUE_MAX,         GTK_ACCESSIBLE_COLLECT_NUMBER,  "valuemax",         (GCallback) gtk_number_accessible_value_new },
  { GTK_ACCESSIBLE_PROPERTY_VALUE_MIN,         GTK_ACCESSIBLE_COLLECT_NUMBER,  "valuemin",         (GCallback) gtk_number_accessible_value_new },
  { GTK_ACCESSIBLE_PROPERTY_VALUE_NOW,         GTK_ACCESSIBLE_COLLECT_NUMBER,  "valuenow",         (GCallback) gtk_number_accessible_value_new },
  { GTK_ACCESSIBLE_PROPERTY_VALUE_TEXT,        GTK_ACCESSIBLE_COLLECT_STRING,  "valuetext",        (GCallback) gtk_string_accessible_value_new },
};

typedef GtkAccessibleValue * (* GtkAccessibleValueBooleanCtor)  (gboolean value);
typedef GtkAccessibleValue * (* GtkAccessibleValueIntCtor)      (int value);
typedef GtkAccessibleValue * (* GtkAccessibleValueTristateCtor) (int value);
typedef GtkAccessibleValue * (* GtkAccessibleValueEnumCtor)     (int value);
typedef GtkAccessibleValue * (* GtkAccessibleValueNumberCtor)   (double value);
typedef GtkAccessibleValue * (* GtkAccessibleValueStringCtor)   (const char *value);
typedef GtkAccessibleValue * (* GtkAccessibleValueRefCtor)      (gpointer value);

/*< private >
 * gtk_accessible_value_get_default_for_state:
 * @state: a #GtkAccessibleState
 *
 * Retrieves the #GtkAccessibleValue that contains the default for the
 * given @state.
 *
 * Returns: (transfer full): the #GtkAccessibleValue
 */
GtkAccessibleValue *
gtk_accessible_value_get_default_for_state (GtkAccessibleState state)
{
  g_assert (state != GTK_ACCESSIBLE_STATE_NONE);

  const GtkAccessibleCollect *cstate = &collect_states[state];

  switch (cstate->value)
    {
    case GTK_ACCESSIBLE_STATE_BUSY:
    case GTK_ACCESSIBLE_STATE_DISABLED:
    case GTK_ACCESSIBLE_STATE_HIDDEN:
      return gtk_boolean_accessible_value_new (FALSE);

    case GTK_ACCESSIBLE_STATE_CHECKED:
      return gtk_checked_accessible_value_new (GTK_ACCESSIBLE_CHECKED_UNDEFINED);

    case GTK_ACCESSIBLE_STATE_EXPANDED:
      return gtk_expanded_accessible_value_new (GTK_ACCESSIBLE_STATE_UNDEFINED);

    case GTK_ACCESSIBLE_STATE_GRABBED:
      return gtk_grabbed_accessible_value_new (GTK_ACCESSIBLE_STATE_UNDEFINED);

    case GTK_ACCESSIBLE_STATE_INVALID:
      return gtk_invalid_accessible_value_new (GTK_ACCESSIBLE_INVALID_FALSE);

    case GTK_ACCESSIBLE_STATE_PRESSED:
      return gtk_pressed_accessible_value_new (GTK_ACCESSIBLE_PRESSED_UNDEFINED);

    case GTK_ACCESSIBLE_STATE_SELECTED:
      return gtk_selected_accessible_value_new (GTK_ACCESSIBLE_STATE_UNDEFINED);

    default:
      g_critical ("Unknown value for accessible state “%s”", cstate->name);
      break;
    }

  return NULL;
}

/*< private >
 * gtk_accessible_value_collect_for_state:
 * @state: a #GtkAccessibleState
 * @args: a `va_list` reference
 *
 * Collects and consumes the next item in the @args variadic arguments list,
 * and returns a #GtkAccessibleValue for it.
 *
 * Returns: (transfer full): a #GtkAccessibleValue
 */
GtkAccessibleValue *
gtk_accessible_value_collect_for_state (GtkAccessibleState  state,
                                        va_list            *args)
{
  g_assert (state != GTK_ACCESSIBLE_STATE_NONE);

  const GtkAccessibleCollect *cstate = &collect_states[state];

  GtkAccessibleValue *res = NULL;

  switch (cstate->ctype)
    {
    case GTK_ACCESSIBLE_COLLECT_BOOLEAN:
      {
        GtkAccessibleValueBooleanCtor ctor =
          (GtkAccessibleValueBooleanCtor) cstate->ctor;

        gboolean value = va_arg (*args, gboolean);

        res = (* ctor) (value);
      }
      break;

    case GTK_ACCESSIBLE_COLLECT_INT:
      {
        GtkAccessibleValueIntCtor ctor =
          (GtkAccessibleValueIntCtor) cstate->ctor;

        int value = va_arg (*args, int);

        res = (* ctor) (value);
      }
      break;

    case GTK_ACCESSIBLE_COLLECT_TRISTATE:
      {
        GtkAccessibleValueTristateCtor ctor =
          (GtkAccessibleValueIntCtor) cstate->ctor;

        int value = va_arg (*args, int);

        res = (* ctor) (value);
      }
      break;

    case GTK_ACCESSIBLE_COLLECT_ENUM:
      {
        GtkAccessibleValueEnumCtor ctor =
          (GtkAccessibleValueEnumCtor) cstate->ctor;

        int value = va_arg (*args, int);

        res = (* ctor) (value);
      }
      break;

    case GTK_ACCESSIBLE_COLLECT_NUMBER:
      {
        GtkAccessibleValueNumberCtor ctor =
          (GtkAccessibleValueNumberCtor) cstate->ctor;

        double value = va_arg (*args, double);

        res = (* ctor) (value);
      }
      break;

    case GTK_ACCESSIBLE_COLLECT_STRING:
      {
        GtkAccessibleValueStringCtor ctor =
          (GtkAccessibleValueStringCtor) cstate->ctor;

        const char *value = va_arg (*args, char*);

        res = (* ctor) (value);
      }
      break;

    case GTK_ACCESSIBLE_COLLECT_REF:
      {
        GtkAccessibleValueStringCtor ctor =
          (GtkAccessibleValueStringCtor) cstate->ctor;

        gpointer value = va_arg (*args, gpointer);

        res = (* ctor) (value);
      }
      break;

    case GTK_ACCESSIBLE_COLLECT_INVALID:
    default:
      g_critical ("Unknown type for accessible state “%s”", cstate->name);
      break;
    }

  return res;
}

/*< private >
 * gtk_accessible_value_collect_for_state_value:
 * @state: a #GtkAccessibleState
 * @value: a #GValue
 *
 * Retrieves the value stored inside @value and returns a #GtkAccessibleValue
 * for the given @state.
 *
 * Returns: (transfer full): a #GtkAccessibleValue
 */
GtkAccessibleValue *
gtk_accessible_value_collect_for_state_value (GtkAccessibleState  state,
                                              const GValue       *value)
{
  g_assert (state != GTK_ACCESSIBLE_STATE_NONE);

  const GtkAccessibleCollect *cstate = &collect_states[state];

  GtkAccessibleValue *res = NULL;

  switch (cstate->ctype)
    {
    case GTK_ACCESSIBLE_COLLECT_BOOLEAN:
      {
        GtkAccessibleValueBooleanCtor ctor =
          (GtkAccessibleValueBooleanCtor) cstate->ctor;

        gboolean value_ = g_value_get_boolean (value);

        res = (* ctor) (value_);
      }
      break;

    case GTK_ACCESSIBLE_COLLECT_INT:
      {
        GtkAccessibleValueIntCtor ctor =
          (GtkAccessibleValueIntCtor) cstate->ctor;

        int value_ = g_value_get_int (value);

        res = (* ctor) (value_);
      }
      break;

    case GTK_ACCESSIBLE_COLLECT_TRISTATE:
      {
        GtkAccessibleValueTristateCtor ctor =
          (GtkAccessibleValueIntCtor) cstate->ctor;

        int value_ = g_value_get_int (value);

        res = (* ctor) (value_);
      }
      break;

    case GTK_ACCESSIBLE_COLLECT_ENUM:
      {
        GtkAccessibleValueEnumCtor ctor =
          (GtkAccessibleValueEnumCtor) cstate->ctor;

        int value_ = g_value_get_enum (value);

        res = (* ctor) (value_);
      }
      break;

    case GTK_ACCESSIBLE_COLLECT_NUMBER:
      {
        GtkAccessibleValueNumberCtor ctor =
          (GtkAccessibleValueNumberCtor) cstate->ctor;

        double value_ = g_value_get_double (value);

        res = (* ctor) (value_);
      }
      break;

    case GTK_ACCESSIBLE_COLLECT_STRING:
      {
        GtkAccessibleValueStringCtor ctor =
          (GtkAccessibleValueStringCtor) cstate->ctor;

        const char *value_ = g_value_get_string (value);

        res = (* ctor) (value_);
      }
      break;

    case GTK_ACCESSIBLE_COLLECT_REF:
      {
        GtkAccessibleValueStringCtor ctor =
          (GtkAccessibleValueStringCtor) cstate->ctor;

        gpointer value_ = g_value_get_object (value);

        res = (* ctor) (value_);
      }
      break;

    case GTK_ACCESSIBLE_COLLECT_INVALID:
    default:
      g_critical ("Unknown value type for accessible state “%s”", cstate->name);
      break;
    }

  return res;
}

/*< private >
 * gtk_accessible_value_get_default_for_property:
 * @property: a #GtkAccessibleProperty
 *
 * Retrieves the #GtkAccessibleValue that contains the default for the
 * given @property.
 *
 * Returns: (transfer full): the #GtkAccessibleValue
 */
GtkAccessibleValue *
gtk_accessible_value_get_default_for_property (GtkAccessibleProperty property)
{
  const GtkAccessibleCollect *cstate = &collect_props[property];

  switch (cstate->value)
    {
    /* Reference properties */
    case GTK_ACCESSIBLE_PROPERTY_ACTIVE_DESCENDANT:
    case GTK_ACCESSIBLE_PROPERTY_CONTROLS:
    case GTK_ACCESSIBLE_PROPERTY_DESCRIBED_BY:
    case GTK_ACCESSIBLE_PROPERTY_FLOW_TO:
    case GTK_ACCESSIBLE_PROPERTY_LABELLED_BY:
    case GTK_ACCESSIBLE_PROPERTY_OWNS:
    case GTK_ACCESSIBLE_PROPERTY_RELEVANT:
      return NULL;

    /* Boolean properties */
    case GTK_ACCESSIBLE_PROPERTY_HAS_POPUP:
    case GTK_ACCESSIBLE_PROPERTY_MULTI_LINE:
    case GTK_ACCESSIBLE_PROPERTY_MULTI_SELECTABLE:
    case GTK_ACCESSIBLE_PROPERTY_READ_ONLY:
    case GTK_ACCESSIBLE_PROPERTY_REQUIRED:
      return gtk_boolean_accessible_value_new (FALSE);

    /* Integer properties */
    case GTK_ACCESSIBLE_PROPERTY_LEVEL:
    case GTK_ACCESSIBLE_PROPERTY_POS_IN_SET:
    case GTK_ACCESSIBLE_PROPERTY_SET_SIZE:
      return gtk_int_accessible_value_new (0);

    /* Number properties */
    case GTK_ACCESSIBLE_PROPERTY_VALUE_MAX:
    case GTK_ACCESSIBLE_PROPERTY_VALUE_MIN:
    case GTK_ACCESSIBLE_PROPERTY_VALUE_NOW:
      return gtk_number_accessible_value_new (0);

    /* String properties */
    case GTK_ACCESSIBLE_PROPERTY_LABEL:
    case GTK_ACCESSIBLE_PROPERTY_VALUE_TEXT:
      return gtk_string_accessible_value_new ("");

    /* Token properties */
    case GTK_ACCESSIBLE_PROPERTY_AUTOCOMPLETE:
      return gtk_autocomplete_accessible_value_new (GTK_ACCESSIBLE_AUTOCOMPLETE_NONE);

    case GTK_ACCESSIBLE_PROPERTY_ORIENTATION:
      return gtk_orientation_accessible_value_new (GTK_ORIENTATION_HORIZONTAL);

    case GTK_ACCESSIBLE_PROPERTY_SORT:
      return gtk_sort_accessible_value_new (GTK_ACCESSIBLE_SORT_NONE);

    default:
      g_critical ("Unknown value for accessible state “%s”", cstate->name);
      break;
    }

  return NULL;
}

/*< private >
 * gtk_accessible_value_collect_for_property:
 * @property: a #GtkAccessibleProperty
 * @args: a `va_list` reference
 *
 * Collects and consumes the next item in the @args variadic arguments list,
 * and returns a #GtkAccessibleValue for it.
 *
 * Returns: (transfer full): a #GtkAccessibleValue
 */
GtkAccessibleValue *
gtk_accessible_value_collect_for_property (GtkAccessibleProperty  property,
                                           va_list               *args)
{
  const GtkAccessibleCollect *cstate = &collect_props[property];

  GtkAccessibleValue *res = NULL;

  switch (cstate->ctype)
    {
    case GTK_ACCESSIBLE_COLLECT_BOOLEAN:
      {
        GtkAccessibleValueBooleanCtor ctor =
          (GtkAccessibleValueBooleanCtor) cstate->ctor;

        gboolean value = va_arg (*args, gboolean);

        res = (* ctor) (value);
      }
      break;

    case GTK_ACCESSIBLE_COLLECT_INT:
      {
        GtkAccessibleValueIntCtor ctor =
          (GtkAccessibleValueIntCtor) cstate->ctor;

        int value = va_arg (*args, int);

        res = (* ctor) (value);
      }
      break;

    case GTK_ACCESSIBLE_COLLECT_TRISTATE:
      {
        GtkAccessibleValueTristateCtor ctor =
          (GtkAccessibleValueIntCtor) cstate->ctor;

        int value = va_arg (*args, int);

        res = (* ctor) (value);
      }
      break;

    case GTK_ACCESSIBLE_COLLECT_ENUM:
      {
        GtkAccessibleValueEnumCtor ctor =
          (GtkAccessibleValueEnumCtor) cstate->ctor;

        int value = va_arg (*args, int);

        res = (* ctor) (value);
      }
      break;

    case GTK_ACCESSIBLE_COLLECT_NUMBER:
      {
        GtkAccessibleValueNumberCtor ctor =
          (GtkAccessibleValueNumberCtor) cstate->ctor;

        double value = va_arg (*args, double);

        res = (* ctor) (value);
      }
      break;

    case GTK_ACCESSIBLE_COLLECT_STRING:
      {
        GtkAccessibleValueStringCtor ctor =
          (GtkAccessibleValueStringCtor) cstate->ctor;

        const char *value = va_arg (*args, char*);

        res = (* ctor) (value);
      }
      break;

    case GTK_ACCESSIBLE_COLLECT_REF:
      {
        GtkAccessibleValueStringCtor ctor =
          (GtkAccessibleValueStringCtor) cstate->ctor;

        gpointer value = va_arg (*args, gpointer);

        res = (* ctor) (value);
      }
      break;

    case GTK_ACCESSIBLE_COLLECT_INVALID:
    default:
      g_critical ("Unknown type for accessible state “%s”", cstate->name);
      break;
    }

  return res;
}

/*< private >
 * gtk_accessible_value_collect_for_property_value:
 * @property: a #GtkAccessibleProperty
 * @value: a #GValue
 *
 * Retrieves the value stored inside @value and returns a #GtkAccessibleValue
 * for the given @property.
 *
 * Returns: (transfer full): a #GtkAccessibleValue
 */
GtkAccessibleValue *
gtk_accessible_value_collect_for_property_value (GtkAccessibleProperty  property,
                                                 const GValue          *value)
{
  const GtkAccessibleCollect *cstate = &collect_props[property];

  GtkAccessibleValue *res = NULL;

  switch (cstate->ctype)
    {
    case GTK_ACCESSIBLE_COLLECT_BOOLEAN:
      {
        GtkAccessibleValueBooleanCtor ctor =
          (GtkAccessibleValueBooleanCtor) cstate->ctor;

        gboolean value_ = g_value_get_boolean (value);

        res = (* ctor) (value_);
      }
      break;

    case GTK_ACCESSIBLE_COLLECT_INT:
      {
        GtkAccessibleValueIntCtor ctor =
          (GtkAccessibleValueIntCtor) cstate->ctor;

        int value_ = g_value_get_int (value);

        res = (* ctor) (value_);
      }
      break;

    case GTK_ACCESSIBLE_COLLECT_TRISTATE:
      {
        GtkAccessibleValueTristateCtor ctor =
          (GtkAccessibleValueIntCtor) cstate->ctor;

        int value_ = g_value_get_int (value);

        res = (* ctor) (value_);
      }
      break;

    case GTK_ACCESSIBLE_COLLECT_ENUM:
      {
        GtkAccessibleValueEnumCtor ctor =
          (GtkAccessibleValueEnumCtor) cstate->ctor;

        int value_ = g_value_get_enum (value);

        res = (* ctor) (value_);
      }
      break;

    case GTK_ACCESSIBLE_COLLECT_NUMBER:
      {
        GtkAccessibleValueNumberCtor ctor =
          (GtkAccessibleValueNumberCtor) cstate->ctor;

        double value_ = g_value_get_double (value);

        res = (* ctor) (value_);
      }
      break;

    case GTK_ACCESSIBLE_COLLECT_STRING:
      {
        GtkAccessibleValueStringCtor ctor =
          (GtkAccessibleValueStringCtor) cstate->ctor;

        const char *value_ = g_value_get_string (value);

        res = (* ctor) (value_);
      }
      break;

    case GTK_ACCESSIBLE_COLLECT_REF:
      {
        GtkAccessibleValueStringCtor ctor =
          (GtkAccessibleValueStringCtor) cstate->ctor;

        gpointer value_ = g_value_get_object (value);

        res = (* ctor) (value_);
      }
      break;

    case GTK_ACCESSIBLE_COLLECT_INVALID:
    default:
      g_critical ("Unknown value type for accessible state “%s”", cstate->name);
      break;
    }

  return res;
}

/* }}} */