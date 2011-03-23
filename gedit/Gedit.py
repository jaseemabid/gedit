from gi.repository import GObject
from ..overrides import override
from ..importer import modules

Gedit = modules['Gedit']._introspection_module
__all__ = []

class MessageBus(Gedit.MessageBus):
    def create(self, object_path, method, **kwargs):
        tp = self.lookup(object_path, method)

        if not tp.is_a(Gedit.Message.__gtype__):
            return None

        kwargs['object-path'] = object_path
        kwargs['method'] = method

        return GObject.new(tp, **kwargs)

    def send_sync(self, object_path, method, **kwargs):
        msg = self.create(object_path, method, **kwargs)
        self.send_message_sync(msg)

        return msg

    def send(self, object_path, method, **kwargs):
        msg = self.create(object_path, method, **kwargs)
        self.send_message(msg)

        return msg

MessageBus = override(MessageBus)
__all__.append('MessageBus')

class Message(Gedit.Message):
    def __getattribute__(self, name):
        try:
            return Gedit.Message.__getattribute__(self, name)
        except:
            return getattr(self.props, name)

Message = override(Message)
__all__.append('Message')

# vi:ex:ts=4:et
