from celery import Celery, Task
from flask import Flask

def celery_init_app(app: Flask) -> Celery:
    class FlaskTask(Task):
        def __call__(self, *args: object, **kwargs: object) -> object:
            with app.app_context():
                return self.run(*args, **kwargs)

    celery_app = Celery(app.name, task_cls=FlaskTask)
    celery_app.config_from_object('celery_config')
    celery_app.set_default()
    app.extensions["celery"] = celery_app
    return celery_app

@celery_app.task
def scheduled_job():
    print("Executing scheduled job for receiving serial data.")
    # Call your function to receive serial data or add any other logic here
