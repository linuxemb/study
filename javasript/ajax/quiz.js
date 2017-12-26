//global var questionnumber is the current 
var questionNumber=0

// load the questions from th eXML file

function getQeustions() {
	obj=document.getElementById("question");
	obj.firstChild.nodeValue=("please wait");
	ajaxCallback = nextQuestion;
	ajaxRequest("questions.xml");
//	console.log(getQuestion().length);
}

//display the next question
function nextQuestion() {
	questions= ajaxreq.responseXML.getElementsByTagName("question");
	obj=document.getElementById("question");
	//console.log(getQuestion.lengh)

	if(questionNumber < questions.length) {
		question = questions[questionNumber].firstChild.nodeValue;
		obj.firstChild.nodeValue=question;
		console.log(question);

	}
	else{
		obj.firstChild.nodeValue="(no more question)";
		console.log("no more questions");

	}
	// body...
}
// check the user anser
function checkAnswer() {
	
	answers= ajaxreq.responseXML.getElementsByTagName("answer");
	answer = answers[questionNumber].firstChild.nodeValue;
	answerfield = document.getElementById("answer");
	if(answer == answerfield.value) {
		alert("Correct !");

	}
	else {
		alert("Incorrct the corect anser is "+ answer);

	}
	questionNumber = questionNumber +1;
	answerfield.value ="";
	nextQuestion();
}

//Set up the event handers for th ebutton
obj = document.getElementById("start_quiz");
obj.onclick= getQeustions;
ans=document.getElementById("submit");
ans.onclick=checkAnswer;



